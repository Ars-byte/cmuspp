#pragma once
/*
  backend/metadata.hpp
  Lightweight tag + duration scanner for the playlist view.

  Thread-safety: read_meta() only touches local state and libsndfile
  (independent handles), so it can run from a background loader thread.
  All parsers are "first-N-bytes" scans (ID3v2 header, first FLAC block,
  first Ogg page) plus a single libsndfile open for exact duration —
  opening a few hundred files takes well under a second.
*/

#include <sndfile.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct TrackMeta {
    std::string artist, album, title;
    double duration = 0.0;   // seconds (0 = unknown)
    int    bitrate  = 0;     // kbps   (0 = unknown)
};

// ── UTF-16 → UTF-8 (BOM-aware) ───────────────────────────────────────────────
static inline std::string md_utf16_to_utf8(const uint8_t* p, size_t len,
                                           bool big_endian) {
    size_t i = 0;
    bool be = big_endian;
    if (len >= 2) {
        if      (p[0] == 0xFF && p[1] == 0xFE) { be = false; i = 2; }
        else if (p[0] == 0xFE && p[1] == 0xFF) { be = true;  i = 2; }
    }
    std::string out;
    out.reserve(len / 2 + 1);
    for (; i + 1 < len; i += 2) {
        uint32_t cp = be ? ((uint32_t)p[i] << 8 | p[i + 1])
                         : ((uint32_t)p[i + 1] << 8 | p[i]);
        if (cp == 0) break; // terminator
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 3 < len) { // surrogate pair
            uint32_t lo = be ? ((uint32_t)p[i + 2] << 8 | p[i + 3])
                             : ((uint32_t)p[i + 3] << 8 | p[i + 2]);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            }
        }
        if      (cp < 0x80)       out += (char)cp;
        else if (cp < 0x800)      { out += (char)(0xC0 | (cp >> 6));
                                    out += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000)    { out += (char)(0xE0 | (cp >> 12));
                                    out += (char)(0x80 | ((cp >> 6) & 0x3F));
                                    out += (char)(0x80 | (cp & 0x3F)); }
        else                      { out += (char)(0xF0 | (cp >> 18));
                                    out += (char)(0x80 | ((cp >> 12) & 0x3F));
                                    out += (char)(0x80 | ((cp >> 6) & 0x3F));
                                    out += (char)(0x80 | (cp & 0x3F)); }
    }
    return out;
}

static inline std::string md_trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e-1] == ' ' || s[e-1] == '\t' ||
                     s[e-1] == '\n' || s[e-1] == '\r')) --e;
    return s.substr(b, e - b);
}

// ── ID3v2 text frame decode ─────────────────────────────────────────────────
static inline std::string md_id3_text(const uint8_t* p, size_t len) {
    if (len < 2) return {};
    uint8_t enc = p[0];
    std::string s;
    if      (enc == 0x00) { // ISO-8859-1
        s.assign((const char*)p + 1, len - 1);
        size_t z = s.find('\0'); if (z != std::string::npos) s = s.substr(0, z);
    } else if (enc == 0x01) { // UTF-16 with BOM
        s = md_utf16_to_utf8(p + 1, len - 1, false);
    } else if (enc == 0x02) { // UTF-16BE (no BOM)
        s = md_utf16_to_utf8(p + 1, len - 1, true);
    } else if (enc == 0x03) { // UTF-8
        s.assign((const char*)p + 1, len - 1);
        size_t z = s.find('\0'); if (z != std::string::npos) s = s.substr(0, z);
    }
    return md_trim(s);
}

static inline uint32_t md_id3_size(const uint8_t* p) {
    return ((uint32_t)(p[0] & 0x7F) << 21) | ((uint32_t)(p[1] & 0x7F) << 14)
         | ((uint32_t)(p[2] & 0x7F) <<  7) | ((uint32_t)(p[3] & 0x7F));
}

// Extract TPE1/TALB/TIT2 (v2.3/2.4) or TP1/TAL/TT2 (v2.2) into `m`.
static inline void md_id3_tags(const std::string& path, TrackMeta& m) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;
    uint8_t hdr[10];
    if (fread(hdr, 1, 10, f) != 10 ||
        hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') { fclose(f); return; }

    size_t tag_size = md_id3_size(hdr + 6);
    if (tag_size == 0 || tag_size > 32 * 1024 * 1024) { fclose(f); return; }
    bool unsync = (hdr[5] & 0x80) != 0;

    std::vector<uint8_t> tag(tag_size);
    size_t got = fread(tag.data(), 1, tag_size, f);
    fclose(f);
    tag.resize(got);

    std::vector<uint8_t> buf;
    const uint8_t* p; size_t left;
    if (unsync) {
        buf.reserve(tag.size());
        for (size_t i = 0; i < tag.size(); ++i) {
            buf.push_back(tag[i]);
            if (tag[i] == 0xFF && i + 1 < tag.size() && tag[i + 1] == 0x00) ++i;
        }
        p = buf.data(); left = buf.size();
    } else {
        p = tag.data(); left = tag.size();
    }

    const bool v22 = (hdr[3] == 2);
    while (left >= (v22 ? 6u : 10u)) {
        if (p[0] == 0) break;
        std::string fid((const char*)p, v22 ? 3 : 4);
        uint32_t fsz;
        if (v22) {
            fsz = ((uint32_t)p[3] << 16) | ((uint32_t)p[4] << 8) | p[5];
            p += 6; left -= 6;
        } else {
            fsz = (hdr[3] >= 4) ? md_id3_size(p + 4)
                                : ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16)
                                | ((uint32_t)p[6] << 8) | p[7];
            p += 10; left -= 10;
        }
        if (fsz > left) break;
        if (fsz > 2) {
            if      (fid == "TPE1" || fid == "TP1") m.artist = md_id3_text(p, fsz);
            else if (fid == "TALB" || fid == "TAL") m.album  = md_id3_text(p, fsz);
            else if (fid == "TIT2" || fid == "TT2") m.title  = md_id3_text(p, fsz);
        }
        p += fsz; left -= fsz;
    }
}

// ── FLAC VORBIS_COMMENT block ────────────────────────────────────────────────
static inline uint32_t md_flac_u32(const std::vector<uint8_t>& d, size_t off) {
    if (off + 4 > d.size()) return 0;
    return ((uint32_t)d[off] << 24) | ((uint32_t)d[off + 1] << 16)
         | ((uint32_t)d[off + 2] << 8) | d[off + 3];
}

static inline bool md_icase_eq(const std::string& a, const char* b) {
    for (size_t i = 0; b[i]; ++i)
        if (i >= a.size() || tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return false;
    return a.size() == strlen(b);
}

static inline void md_apply_keyval(TrackMeta& m, const std::string& kv) {
    size_t eq = kv.find('=');
    if (eq == std::string::npos) return;
    std::string k = kv.substr(0, eq);
    std::string v = md_trim(kv.substr(eq + 1));
    if (v.empty()) return;
    if      (md_icase_eq(k, "ARTIST")) m.artist = v;
    else if (md_icase_eq(k, "ALBUM"))  m.album  = v;
    else if (md_icase_eq(k, "TITLE"))  m.title  = v;
}

static inline void md_flac_comments(const std::string& path, TrackMeta& m) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;
    uint8_t sig[4];
    if (fread(sig, 1, 4, f) < 4 || memcmp(sig, "fLaC", 4) != 0) { fclose(f); return; }

    for (int blk = 0; blk < 64; ++blk) {
        uint8_t hdr[4];
        if (fread(hdr, 1, 4, f) < 4) break;
        bool last = (hdr[0] & 0x80) != 0;
        int  type = hdr[0] & 0x7F;
        uint32_t sz = ((uint32_t)hdr[1] << 16) | ((uint32_t)hdr[2] << 8) | hdr[3];
        if (type == 4 && sz > 4) { // VORBIS_COMMENT
            std::vector<uint8_t> d(sz);
            if (fread(d.data(), 1, sz, f) != sz) { fclose(f); return; }
            size_t off = 0;
            uint32_t vendor_len = md_flac_u32(d, off); off += 4;
            off += vendor_len;
            if (off + 4 > d.size()) { fclose(f); return; }
            uint32_t n = md_flac_u32(d, off); off += 4;
            for (uint32_t i = 0; i < n && off + 4 <= d.size(); ++i) {
                uint32_t cl = md_flac_u32(d, off); off += 4;
                if (off + cl > d.size()) break;
                md_apply_keyval(m, std::string((const char*)d.data() + off, cl));
                off += cl;
            }
            fclose(f); return;
        }
        if (fseek(f, sz, SEEK_CUR) != 0) break;
        if (last) break;
    }
    fclose(f);
}

// ── OGG / OPUS comments (first page scan) ────────────────────────────────────
static inline void md_ogg_tags(const std::string& path, TrackMeta& m) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return;
    std::vector<char> buf(65536);
    size_t nr = fread(buf.data(), 1, buf.size(), f);
    fclose(f);
    if (nr < 8) return;

    std::string s(buf.data(), nr);
    std::string low = s;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);

    auto find_val = [&](const char* key) -> std::string {
        std::string kl(key);
        std::transform(kl.begin(), kl.end(), kl.begin(), ::tolower);
        size_t p = low.find(kl);
        while (p != std::string::npos) {
            size_t st = p + kl.size();
            size_t e  = low.find('\n', st);
            std::string v = s.substr(st, (e == std::string::npos)
                                        ? std::string::npos : e - st);
            v = md_trim(v);
            if (!v.empty()) return v;
            p = low.find(kl, p + kl.size());
        }
        return {};
    };

    m.artist = find_val("ARTIST=");
    m.album  = find_val("ALBUM=");
    m.title  = find_val("TITLE=");
}

// ── Master entry point ───────────────────────────────────────────────────────
inline TrackMeta read_meta(const std::string& path) {
    TrackMeta m;

    std::string lc = path;
    std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);

    if      (lc.size() > 4 && lc.compare(lc.size() - 4, 4, ".mp3") == 0)
        md_id3_tags(path, m);
    else if (lc.size() > 5 && lc.compare(lc.size() - 5, 5, ".flac") == 0)
        md_flac_comments(path, m);
    else if (lc.size() > 4 &&
             (lc.compare(lc.size() - 4, 4, ".ogg") == 0 ||
              lc.compare(lc.size() - 5, 5, ".opus") == 0))
        md_ogg_tags(path, m);

    // Exact duration + derived bitrate via libsndfile (uniform, VBR-safe).
    SF_INFO info{};
    SNDFILE* sf = sf_open(path.c_str(), SFM_READ, &info);
    if (sf) {
        if (info.samplerate > 0 && info.frames > 0)
            m.duration = (double)info.frames / info.samplerate;
        sf_close(sf);
    }
    if (m.duration > 0) {
        long fsz = 0;
        std::error_code ec;
        fsz = (long)fs::file_size(path, ec);
        if (fsz > 0)
            m.bitrate = (int)((double)fsz * 8.0 / m.duration / 1000.0 + 0.5);
    }
    return m;
}
