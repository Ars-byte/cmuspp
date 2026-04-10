#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#ifdef CMUSPP_HAS_JPEG
#  include <jpeglib.h>
#  include <setjmp.h>
#endif

#ifdef CMUSPP_HAS_PNG
#  include <png.h>
#endif

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

// ── Basic types ───────────────────────────────────────────────────────────────
struct Pixel { uint8_t r, g, b; };

struct RawImg {
    std::vector<Pixel> px;
    int w = 0, h = 0;
    bool empty() const { return px.empty(); }
    void free_pixels() { std::vector<Pixel>().swap(px); w = 0; h = 0; }
};

enum class ImgFmt { NONE, JPEG, PNG };

inline ImgFmt detect_fmt(const uint8_t* d, size_t len) {
    if (len >= 3 && d[0]==0xFF && d[1]==0xD8 && d[2]==0xFF) return ImgFmt::JPEG;
    if (len >= 4 && d[0]==0x89 && d[1]=='P'  && d[2]=='N'  && d[3]=='G') return ImgFmt::PNG;
    return ImgFmt::NONE;
}
inline ImgFmt mime_to_fmt(const std::string& m) {
    if (m.find("jpeg")!=std::string::npos || m.find("jpg")!=std::string::npos) return ImgFmt::JPEG;
    if (m.find("png") !=std::string::npos) return ImgFmt::PNG;
    return ImgFmt::NONE;
}

// ── Image decoders ────────────────────────────────────────────────────────────
#ifdef CMUSPP_HAS_JPEG
struct JpegErr { struct jpeg_error_mgr pub; jmp_buf jb; };
static void jpeg_err_exit(j_common_ptr c) { longjmp(((JpegErr*)c->err)->jb, 1); }

inline RawImg decode_jpeg(const uint8_t* data, size_t len) {
    RawImg img;
    struct jpeg_decompress_struct ci{};
    JpegErr je{};
    ci.err = jpeg_std_error(&je.pub);
    je.pub.error_exit = jpeg_err_exit;
    if (setjmp(je.jb)) { jpeg_destroy_decompress(&ci); return {}; }
    jpeg_create_decompress(&ci);
    jpeg_mem_src(&ci, data, (unsigned long)len);
    if (jpeg_read_header(&ci, TRUE) != JPEG_HEADER_OK) { jpeg_destroy_decompress(&ci); return {}; }
    ci.out_color_space = JCS_RGB;
    if (ci.image_width > 400 || ci.image_height > 400) ci.scale_denom = 4;
    else if (ci.image_width > 200 || ci.image_height > 200) ci.scale_denom = 2;
    jpeg_start_decompress(&ci);
    img.w = (int)ci.output_width; img.h = (int)ci.output_height;
    img.px.resize((size_t)img.w * img.h);
    std::vector<uint8_t> row((size_t)img.w * 3);
    JSAMPROW rp = row.data();
    for (int y = 0; (int)ci.output_scanline < img.h; ++y) {
        jpeg_read_scanlines(&ci, &rp, 1);
        for (int x = 0; x < img.w; ++x)
            img.px[(size_t)y*img.w+x] = {row[x*3], row[x*3+1], row[x*3+2]};
    }
    jpeg_finish_decompress(&ci); jpeg_destroy_decompress(&ci);
    return img;
}
#else
inline RawImg decode_jpeg(const uint8_t*, size_t) { return {}; }
#endif

#ifdef CMUSPP_HAS_PNG
inline RawImg decode_png(const uint8_t* data, size_t len) {
    if (len < 8 || png_sig_cmp(data, 0, 8)) return {};
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);
    if (!png) return {};
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png,nullptr,nullptr); return {}; }
    struct Src { const uint8_t* p; size_t rem; };
    Src src{data+8, len-8};
    png_set_read_fn(png, &src,[](png_structp ps, png_bytep buf, png_size_t n){
        Src* s = (Src*)png_get_io_ptr(ps);
        size_t r = std::min(n, s->rem);
        memcpy(buf, s->p, r); s->p += r; s->rem -= r;
    });
    png_set_sig_bytes(png, 8);
    if (setjmp(png_jmpbuf(png))) { png_destroy_read_struct(&png,&info,nullptr); return {}; }
    png_read_info(png, info);
    int w=(int)png_get_image_width(png,info), h=(int)png_get_image_height(png,info);
    int ct=png_get_color_type(png,info), bd=png_get_bit_depth(png,info);
    if (bd==16) png_set_strip_16(png);
    if (ct==PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (ct==PNG_COLOR_TYPE_GRAY && bd<8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png,info,PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (ct & PNG_COLOR_MASK_ALPHA) png_set_strip_alpha(png);
    if (ct==PNG_COLOR_TYPE_GRAY || ct==PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
    png_read_update_info(png,info);
    RawImg img; img.w=w; img.h=h; img.px.resize((size_t)w*h);
    std::vector<std::vector<uint8_t>> rows(h, std::vector<uint8_t>((size_t)w*3));
    std::vector<png_bytep> rp(h);
    for (int y=0;y<h;++y) rp[y]=rows[y].data();
    png_read_image(png, rp.data());
    png_destroy_read_struct(&png,&info,nullptr);
    for (int y=0;y<h;++y)
        for (int x=0;x<w;++x)
            img.px[(size_t)y*w+x]={rows[y][x*3],rows[y][x*3+1],rows[y][x*3+2]};
    return img;
}
#else
inline RawImg decode_png(const uint8_t*, size_t) { return {}; }
#endif

inline RawImg decode_image(const uint8_t* d, size_t len, ImgFmt hint=ImgFmt::NONE) {
    ImgFmt fmt = detect_fmt(d, len);
    if (fmt == ImgFmt::NONE) fmt = hint;
    if (fmt == ImgFmt::JPEG) return decode_jpeg(d, len);
    if (fmt == ImgFmt::PNG)  return decode_png(d, len);
    return {};
}

// ── Area-averaging resize ─────────────────────────────────────────────────────
inline RawImg resize_img(const RawImg& src, int tw, int th) {
    if (src.w == tw && src.h == th) return src;
    if (tw <= 0 || th <= 0 || src.w <= 0 || src.h <= 0) return {};
    RawImg dst; dst.w = tw; dst.h = th; dst.px.resize((size_t)tw * th);
    for (int y = 0; y < th; ++y) {
        int sy1 = y * src.h / th, sy2 = std::max(sy1+1, (y+1)*src.h/th);
        if (sy2 > src.h) sy2 = src.h;
        for (int x = 0; x < tw; ++x) {
            int sx1 = x * src.w / tw, sx2 = std::max(sx1+1, (x+1)*src.w/tw);
            if (sx2 > src.w) sx2 = src.w;
            long r=0,g=0,b=0,n=0;
            for (int cy=sy1;cy<sy2;++cy)
                for (int cx=sx1;cx<sx2;++cx) {
                    const Pixel& p=src.px[(size_t)cy*src.w+cx];
                    r+=p.r; g+=p.g; b+=p.b; ++n;
                }
            if (n) dst.px[(size_t)y*tw+x]={(uint8_t)(r/n),(uint8_t)(g/n),(uint8_t)(b/n)};
        }
    }
    return dst;
}

// ── Trivial uncompressed PNG encoder ─────────────────────────────────────────
namespace png_enc {
static const uint32_t* crc_table() {
    static uint32_t T[256];
    static bool init = false;
    if (!init) {
        for (uint32_t n=0;n<256;++n) {
            uint32_t c=n;
            for (int k=0;k<8;++k) c = (c&1) ? (0xEDB88320u ^ (c>>1)) : (c>>1);
            T[n]=c;
        }
        init=true;
    }
    return T;
}
static uint32_t crc32(const uint8_t* d, size_t n) {
    const uint32_t* T = crc_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i=0;i<n;++i) crc = T[(crc^d[i])&0xFF] ^ (crc>>8);
    return crc ^ 0xFFFFFFFFu;
}
static void u32be(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back((v>>24)&0xFF); out.push_back((v>>16)&0xFF);
    out.push_back((v>> 8)&0xFF); out.push_back( v     &0xFF);
}
static void chunk(std::vector<uint8_t>& out, const char* type,
                  const uint8_t* data, size_t len) {
    u32be(out, (uint32_t)len);
    out.insert(out.end(), type, type+4);
    if (data && len) out.insert(out.end(), data, data+len);
    std::vector<uint8_t> td(4 + len);
    memcpy(td.data(), type, 4);
    if (data && len) memcpy(td.data()+4, data, len);
    u32be(out, crc32(td.data(), td.size()));
}
static std::vector<uint8_t> deflate_store(const uint8_t* in, size_t len) {
    std::vector<uint8_t> out;
    out.reserve(len + (len/65535+1)*5 + 6 + 4);
    out.push_back(0x78); out.push_back(0x01);
    size_t pos = 0;
    while (true) {
        size_t blk = std::min((size_t)65535, len - pos);
        bool last = (pos + blk >= len);
        out.push_back(last ? 0x01 : 0x00);
        uint16_t L = (uint16_t)blk, NL = ~L;
        out.push_back(L&0xFF); out.push_back(L>>8);
        out.push_back(NL&0xFF); out.push_back(NL>>8);
        out.insert(out.end(), in+pos, in+pos+blk);
        pos += blk;
        if (last) break;
    }
    uint32_t s1=1, s2=0;
    for (size_t i=0;i<len;++i) { s1=(s1+in[i])%65521; s2=(s2+s1)%65521; }
    u32be(out, (s2<<16)|s1);
    return out;
}
inline std::vector<uint8_t> encode(const RawImg& img) {
    std::vector<uint8_t> out;
    out.reserve((size_t)img.w * img.h * 3 + img.h + 200);
    static const uint8_t sig[] = {137,80,78,71,13,10,26,10};
    out.insert(out.end(), sig, sig+8);
    {
        uint8_t ihdr[13] = {};
        ihdr[0]=(img.w>>24)&0xFF; ihdr[1]=(img.w>>16)&0xFF;
        ihdr[2]=(img.w>> 8)&0xFF; ihdr[3]= img.w     &0xFF;
        ihdr[4]=(img.h>>24)&0xFF; ihdr[5]=(img.h>>16)&0xFF;
        ihdr[6]=(img.h>> 8)&0xFF; ihdr[7]= img.h     &0xFF;
        ihdr[8]=8; ihdr[9]=2;
        chunk(out, "IHDR", ihdr, 13);
    }
    size_t row_bytes = (size_t)img.w * 3;
    std::vector<uint8_t> raw(img.h * (1 + row_bytes));
    for (int y=0; y<img.h; ++y) {
        raw[y*(1+row_bytes)] = 0;
        for (int x=0; x<img.w; ++x) {
            const Pixel& p = img.px[(size_t)y*img.w+x];
            size_t off = y*(1+row_bytes) + 1 + x*3;
            raw[off]=p.r; raw[off+1]=p.g; raw[off+2]=p.b;
        }
    }
    auto idat = deflate_store(raw.data(), raw.size());
    chunk(out, "IDAT", idat.data(), idat.size());
    chunk(out, "IEND", nullptr, 0);
    return out;
}
} // namespace png_enc

// ── Cover extraction from audio files ────────────────────────────────────────
static inline uint32_t be32(const uint8_t* p){return((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static inline uint32_t be24(const uint8_t* p){return((uint32_t)p[0]<<16)|((uint32_t)p[1]<<8)|p[2];}
static inline uint32_t ss28(const uint8_t* p){return((uint32_t)(p[0]&0x7f)<<21)|((uint32_t)(p[1]&0x7f)<<14)|((uint32_t)(p[2]&0x7f)<<7)|((uint32_t)(p[3]&0x7f));}

inline RawImg extract_mp3_cover(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return {};
    uint8_t hdr[10];
    if (fread(hdr,1,10,f)<10 || hdr[0]!='I'||hdr[1]!='D'||hdr[2]!='3') { fclose(f); return {}; }
    int ver=(int)hdr[3];
    bool unsync=(hdr[5]&0x80)!=0;
    uint32_t tag_size=ss28(hdr+6);
    if (tag_size>16*1024*1024) { fclose(f); return {}; }
    std::vector<uint8_t> tag(tag_size);
    size_t got=fread(tag.data(),1,tag_size,f); fclose(f);
    tag.resize(got);

    std::vector<uint8_t> buf;
    if (unsync) {
        buf.reserve(tag_size);
        for (size_t i=0;i<tag.size();++i) {
            buf.push_back(tag[i]);
            if (tag[i]==0xFF && i+1<tag.size() && tag[i+1]==0x00) ++i;
        }
    }
    const uint8_t* p   = unsync ? buf.data() : tag.data();
    size_t         left = unsync ? buf.size() : tag.size();
    bool v22=(ver==2);

    while (left>=(v22?6u:10u)) {
        if (p[0]==0x00) break;
        std::string fid(reinterpret_cast<const char*>(p), v22?3:4);
        uint32_t fsz;
        if (v22) { fsz=be24(p+3); p+=6; left-=6; }
        else     { fsz=(ver>=4)?ss28(p+4):be32(p+4); p+=10; left-=10; }
        if (fsz>left) break;

        bool is_apic=(!v22&&fid=="APIC")||(v22&&fid=="PIC");
        if (is_apic && fsz>4) {
            const uint8_t* fp=p; size_t fl=fsz;
            fp++; fl--;  
            ImgFmt fmt=ImgFmt::NONE;
            if (v22) {
                if (fl<3) goto next;
                char ext[4]={0}; memcpy(ext,fp,3); fp+=3; fl-=3;
                std::string se(ext); std::transform(se.begin(),se.end(),se.begin(),::tolower);
                if (se=="jpg"||se=="jpe") fmt=ImgFmt::JPEG;
                else if (se=="png")       fmt=ImgFmt::PNG;
            } else {
                while (fl && *fp) { fp++; fl--; }
                if (fl) { fp++; fl--; }
                fmt=ImgFmt::JPEG;
            }
            { uint8_t pt=fp[0]; fp++; fl--;
              if (fl && *fp==0) { fp++; fl--; }
              else while (fl && *fp) { fp++; fl--; }
              if (fl && *fp==0) { fp++; fl--; }
              (void)pt;
            }
            if (fl>4) { RawImg img=decode_image(fp,fl,fmt); if (!img.empty()) return img; }
        }
        next: p+=fsz; left-=fsz;
    }
    return {};
}

inline RawImg extract_flac_cover(const std::string& path) {
    FILE* f=fopen(path.c_str(),"rb");
    if (!f) return {};
    uint8_t sig[4];
    if (fread(sig,1,4,f)<4||memcmp(sig,"fLaC",4)!=0) { fclose(f); return {}; }
    while (true) {
        uint8_t hdr[4];
        if (fread(hdr,1,4,f)<4) break;
        bool last=(hdr[0]&0x80)!=0;
        int  type=hdr[0]&0x7F;
        uint32_t sz=be24(hdr+1);
        if (type==6 && sz>8) {
            std::vector<uint8_t> blk(sz);
            if (fread(blk.data(),1,sz,f)!=sz) break;
            uint32_t pic_type=be32(blk.data());
            if (pic_type==3||pic_type==0) {
                uint32_t mime_len=be32(blk.data()+4);
                if (8+mime_len+4>sz) break;
                std::string mime(blk.begin()+8, blk.begin()+8+mime_len);
                ImgFmt fmt=mime_to_fmt(mime);
                uint32_t desc_len=be32(blk.data()+8+mime_len);
                size_t off=8+mime_len+4+desc_len+16+4;
                uint32_t data_len=be32(blk.data()+off-4);
                if (off+data_len<=sz) {
                    RawImg img=decode_image(blk.data()+off,data_len,fmt);
                    if (!img.empty()) { fclose(f); return img; }
                }
            } else { fseek(f,sz,SEEK_CUR); }
        } else { fseek(f,sz,SEEK_CUR); }
        if (last) break;
    }
    fclose(f); return {};
}

inline std::string base64_decode_str(const std::string& in) {
    static const int8_t T[256]={
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    };
    std::string out; out.reserve(in.size()*3/4);
    int val=0,bits=-8;
    for (unsigned char c:in) {
        if (T[c]==-1) continue;
        val=(val<<6)|(T[c]); bits+=6;
        if (bits>=0) { out.push_back((char)((val>>bits)&0xFF)); bits-=8; }
    }
    return out;
}

inline RawImg extract_ogg_cover(const std::string& path) {
    FILE* f=fopen(path.c_str(),"rb");
    if (!f) return {};
    std::vector<uint8_t> buf(65536);
    size_t nr=fread(buf.data(),1,buf.size(),f); fclose(f);
    buf.resize(nr);
    const char* KEY="METADATA_BLOCK_PICTURE=";
    size_t kl=strlen(KEY);
    for (size_t i=0;i+kl<nr;++i) {
        if (memcmp(buf.data()+i,KEY,kl)==0) {
            size_t start=i+kl, end=start;
            while (end<nr && buf[end]!='\x01' && buf[end]!=0x01 && buf[end]>=' ') ++end;
            if (end<=start) continue;
            std::string b64(buf.begin()+start,buf.begin()+end);
            std::string raw=base64_decode_str(b64);
            const uint8_t* r=(const uint8_t*)raw.data(); size_t rl=raw.size();
            if (rl<32) continue;
            uint32_t mime_len=be32(r+4); if (8+mime_len>rl) continue;
            std::string mime(raw.begin()+8,raw.begin()+8+mime_len);
            uint32_t desc_len=be32(r+8+mime_len);
            size_t off=8+mime_len+4+desc_len+16+4;
            if (off>=rl) continue;
            uint32_t dlen=be32(r+off-4);
            if (off+dlen>rl) continue;
            RawImg img=decode_image(r+off,dlen,mime_to_fmt(mime));
            if (!img.empty()) return img;
        }
    }
    return {};
}

inline RawImg extract_cover_from_dir(const std::string& path) {
    fs::path dir = fs::path(path).parent_path();
    static const char* names[] = {
        "cover.jpg","cover.png","folder.jpg","folder.png",
        "album.jpg","album.png","front.jpg","front.png",
        "artwork.jpg","artwork.png", nullptr
    };
    for (int i=0; names[i]; ++i) {
        fs::path p = dir / names[i];
        FILE* f=fopen(p.string().c_str(),"rb");
        if (!f) continue;
        fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
        if (sz<=0||sz>8*1024*1024) { fclose(f); continue; }
        std::vector<uint8_t> d(sz);
        fread(d.data(),1,sz,f); fclose(f);
        RawImg img=decode_image(d.data(),sz);
        if (!img.empty()) return img;
    }
    return {};
}

inline RawImg extract_cover(const std::string& path) {
    std::string lc=path;
    std::transform(lc.begin(),lc.end(),lc.begin(),::tolower);
    RawImg img;
    if (lc.size()>4 && (lc.compare(lc.size()-4,4,".mp3")==0))
        img=extract_mp3_cover(path);
    else if (lc.size()>5 && lc.compare(lc.size()-5,5,".flac")==0)
        img=extract_flac_cover(path);
    else if (lc.size()>4 && (lc.compare(lc.size()-4,4,".ogg")==0||
                              lc.compare(lc.size()-5,5,".opus")==0))
        img=extract_ogg_cover(path);
    if (img.empty()) img=extract_cover_from_dir(path);
    return img;
}

// ── Terminal capability detection ─────────────────────────────────────────────
inline bool is_kitty() {
    static int cached = -1;
    if (cached >= 0) return cached == 1;

    auto chk_env = [](const char* var, const char* needle) -> bool {
        const char* v = getenv(var);
        return v && strstr(v, needle) != nullptr;
    };

    if (chk_env("TERM", "kitty") || chk_env("TERM_PROGRAM", "kitty") ||
        chk_env("TERM_PROGRAM", "WezTerm") || chk_env("TERM_PROGRAM", "ghostty") ||
        chk_env("TERM_PROGRAM", "iTerm") || chk_env("TERM", "xterm-kitty")) {
        cached = 1; return true;
    }

    if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
        struct termios old, raw;
        tcgetattr(STDIN_FILENO, &old);
        raw = old;
        cfmakeraw(&raw);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        const char* probe = "\033_Ga=a,s=1,v=1,i=9999,q=1;\033\\";
        write(STDOUT_FILENO, probe, strlen(probe));

        struct timeval tv = {0, 80000}; 
        fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
        bool got_response = false;
        if (select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv) > 0) {
            char buf[64] = {};
            read(STDIN_FILENO, buf, sizeof(buf)-1);
            if (strstr(buf, "\033_G") || strstr(buf, "OK") || strstr(buf, "ENOENT"))
                got_response = true;
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        if (got_response) {
            tv = {0, 10000};
            if (select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv) > 0) {
                char junk[128]; read(STDIN_FILENO, junk, sizeof(junk));
            }
            cached = 1; return true;
        }
    }

    cached = 0; return false;
}

struct CellPx { int w, h; };
inline CellPx cell_pixel_size() {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0
        && ws.ws_col > 0 && ws.ws_row > 0
        && ws.ws_xpixel > 0 && ws.ws_ypixel > 0) {
        return { (int)(ws.ws_xpixel / ws.ws_col),
                 (int)(ws.ws_ypixel / ws.ws_row) };
    }
    return {8, 16};
}

// ── Base64 encoder ─────────────────────────────────────────────────────────────
static const char B64T[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline void base64_append(std::string& dst, const uint8_t* data, size_t len) {
    dst.reserve(dst.size() + (len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = ((uint32_t)data[i] << 16)
                   | ((i+1 < len ? (uint32_t)data[i+1] : 0) << 8)
                   | ((i+2 < len ? (uint32_t)data[i+2] : 0));
        dst.push_back(B64T[(n >> 18) & 63]);
        dst.push_back(B64T[(n >> 12) & 63]);
        dst.push_back(i+1 < len ? B64T[(n >> 6) & 63] : '=');
        dst.push_back(i+2 < len ? B64T[n & 63]        : '=');
    }
}

// ── ANSI half-block fallback ───────────────────────────────────────────────────
inline std::vector<std::string> render_cover(const RawImg& img, int cols, int rows) {
    RawImg sized = resize_img(img, cols, rows*2);
    std::vector<std::string> lines; lines.reserve(rows);
    char esc[64];
    for (int ty=0;ty<rows;++ty) {
        std::string line; line.reserve((size_t)cols*28);
        for (int tx=0;tx<cols;++tx) {
            size_t top_idx = (size_t)(ty*2)*cols + tx;
            size_t bot_idx = (ty*2+1 < rows*2) ? (size_t)(ty*2+1)*cols + tx : top_idx;
            Pixel up = sized.px[top_idx];
            Pixel dn = sized.px[bot_idx];
            snprintf(esc,sizeof(esc),"\033[48;2;%d;%d;%dm",up.r,up.g,up.b); line+=esc;
            snprintf(esc,sizeof(esc),"\033[38;2;%d;%d;%dm",dn.r,dn.g,dn.b); line+=esc;
            line += "▄";
        }
        line += "\033[0m";
        lines.push_back(std::move(line));
    }
    return lines;
}

inline std::vector<std::string> render_no_cover(int cols, int rows,
                                                  const char* /*fg2*/,
                                                  const char* fg3,
                                                  const char* acc) {
    std::vector<std::string> lines(rows);
    
    static const char* art[]={
        "⠀⠀⠀⠀⠀⠀⠀⠀⣀⣤⣶⣶⣾⣿⣿⣿⣿⣷⣶⣶⣤⣀⠀⠀⠀⠀⠀⠀⠀⠀",
        "⠀⠀⠀⠀⠀⣠⢔⣫⢷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⣄⠀⠀⠀⠀⠀",
        "⠀⠀⠀⣠⢊⡴⡫⢚⡽⣟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀",
        "⠀⠀⡴⣱⢫⢎⡔⡩⣚⠵⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⠀⠀",
        "⠀⣼⣽⣳⣣⢯⣞⡜⡱⣫⢷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀",
        "⢸⣿⣿⣿⣿⣿⣿⣾⡽⣱⣫⠞⠉⠀⠀⠀⠀⠉⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇",
        "⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠃⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷",
        "⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠘⠃⠀⠀⠀⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿",
        "⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⢀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿",
        "⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣤⣀⣀⣀⣠⣴⢟⡵⣳⢯⢿⣿⡟⣿⣿⣿⣿⡇",
        "⠀⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣞⡵⣫⢏⢞⡽⡽⣻⢯⡟⠀",
        "⠀⠀⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣚⢕⡡⢊⠜⡵⣣⠟⠀⠀",
        "⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣯⢷⣫⢖⡥⢊⡴⠋⠀⠀⠀",
        "⠀⠀⠀⠀⠀⠙⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣞⣭⠞⠋⠀⠀⠀⠀⠀",
        "⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠿⠿⢿⣿⣿⣿⣿⡿⠿⠟⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀",
        "",
        "no artcover, skill issues?",
        nullptr
    };

    int art_rows = 0;
    while (art[art_rows]) ++art_rows;

    int v_pad = std::max(0, (rows - art_rows) / 2);

    for (int y = 0; y < rows; ++y) {
        std::string& line = lines[y];
        line.reserve(cols * 6);

        int ai = y - v_pad;
        if (ai >= 0 && ai < art_rows) {
            const char* a = art[ai];
            int aw = 0;
            for (const char* c = a; *c; ) {
                unsigned char b = (unsigned char)*c;
                int sk = (b < 0x80) ? 1 : (b < 0xE0) ? 2 : (b < 0xF0) ? 3 : 4;
                c += sk; ++aw;
            }
            int lpad = std::max(0, (cols - aw) / 2);
            int rpad = std::max(0, cols - aw - lpad);

            bool is_label = (ai >= art_rows - 2); 
            if (is_label) line += fg3;
            else          line += acc;

            line += std::string(lpad, ' ');

            if (aw <= cols) {
                line += a;
                line += std::string(rpad, ' ');
            } else {
                int remain = cols - lpad;
                int written = 0;
                for (const char* c = a; *c && written < remain; ) {
                    unsigned char b = (unsigned char)*c;
                    int sk = (b < 0x80) ? 1 : (b < 0xE0) ? 2 : (b < 0xF0) ? 3 : 4;
                    for (int k = 0; k < sk; ++k) line += c[k];
                    c += sk; ++written;
                }
            }
        } else {
            line += fg3;
            line += std::string(cols, ' ');
        }
        line += "\033[0m";
    }
    return lines;
}

// ── Cover cache ───────────────────────────────────────────────────────────────
struct CoverCache {
    std::string cached_path;
    int    cached_cols = 0, cached_rows = 0;
    bool   have_img    = false;

    std::string  kitty_b64;      
    RawImg       kitty_src;      

    RawImg                   ansi_img;
    std::vector<std::string> lines;

    std::string get_kitty_seq(
            const std::string& song_path,
            int c, int r,
            const char* fg2, const char* fg3, const char* acc,
            int cover_term_row, int cover_term_col)
    {
        bool new_song    = (song_path != cached_path);
        bool size_change = (c != cached_cols || r != cached_rows);

        if (new_song) {
            cached_path = song_path;
            have_img    = false;
            { std::string t; t.swap(kitty_b64); }
            kitty_src.free_pixels();
            ansi_img.free_pixels();
            lines.clear();
            cached_cols = c; cached_rows = r;

            RawImg img = song_path.empty() ? RawImg{} : extract_cover(song_path);
            have_img = !img.empty();

            if (is_kitty()) {
                if (have_img) {
                    int ts = std::min({img.w, img.h, 128});
                    kitty_src = resize_img(img, ts, ts);
                    img.free_pixels();
                    _rebuild_kitty_b64(c, r);
                    lines.assign(r, std::string(c, ' '));
                } else {
                    lines = render_no_cover(c, r, fg2, fg3, acc);
                }
            } else {
                if (have_img) {
                    ansi_img = std::move(img);
                    lines = render_cover(ansi_img, c, r);
                } else {
                    lines = render_no_cover(c, r, fg2, fg3, acc);
                }
            }
        } else if (size_change) {
            cached_cols = c; cached_rows = r;
            if (is_kitty()) {
                if (have_img) {
                    _rebuild_kitty_b64(c, r);
                    lines.assign(r, std::string(c, ' '));
                } else {
                    lines = render_no_cover(c, r, fg2, fg3, acc);
                }
            } else {
                if (have_img) lines = render_cover(ansi_img, c, r);
                else          lines = render_no_cover(c, r, fg2, fg3, acc);
            }
        }

        if (is_kitty() && have_img && !kitty_b64.empty()) {
            return _build_seq(c, r, cover_term_row, cover_term_col);
        }
        return {};
    }

    const std::vector<std::string>& get_lines() const { return lines; }

private:
    void _rebuild_kitty_b64(int c, int r) {
        CellPx cpx = cell_pixel_size();
        int px_w = c * cpx.w;
        int px_h = r * cpx.h;
        if (kitty_src.w > 0 && kitty_src.h > 0) {
            float src_ar = (float)kitty_src.w / (float)kitty_src.h;
            float dst_ar = (float)px_w / (float)px_h;
            if (src_ar > dst_ar) px_h = std::max(1, (int)(px_w / src_ar));
            else                  px_w = std::max(1, (int)(px_h * src_ar));
        }
        px_w = std::max(1, px_w);
        px_h = std::max(1, px_h);

        RawImg scaled = resize_img(kitty_src, px_w, px_h);
        std::vector<uint8_t> png = png_enc::encode(scaled);
        scaled.free_pixels();

        kitty_b64.clear();
        base64_append(kitty_b64, png.data(), png.size());
    }

    std::string _build_seq(int disp_cols, int disp_rows,
                            int term_row,  int term_col) const {
        std::string seq;
        seq += "\033[";
        seq += std::to_string(term_row);
        seq += ";";
        seq += std::to_string(term_col);
        seq += "H";

        seq += "\033_Ga=d,d=i,i=1,q=2;\033\\";

        static constexpr size_t CHUNK = 4096;
        size_t pos   = 0;
        bool   first = true;
        const size_t total = kitty_b64.size();

        while (pos < total || first) {
            size_t take = (total > pos) ? std::min(CHUNK, total - pos) : 0;
            bool   more = (pos + take < total);

            seq += "\033_G";
            if (first) {
                seq += "a=T,f=100,q=2,i=1,c=";
                seq += std::to_string(disp_cols);
                seq += ",r=";
                seq += std::to_string(disp_rows);
                seq += ",m=";
                seq += (more ? '1' : '0');
                seq += ';';
                first = false;
            } else {
                seq += "m=";
                seq += (more ? '1' : '0');
                seq += ';';
            }
            if (take > 0) seq.append(kitty_b64, pos, take);
            seq += "\033\\";
            
            pos += take;
            if (!more) break;
        }

        seq += "\033[H";
        return seq;
    }
};
