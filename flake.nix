{
  description = "CMUS++ — C++17 terminal music player with cover art, lyrics and themes";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system}; in
        {
          cmuspp = pkgs.stdenv.mkDerivation {
            pname = "cmuspp";
            version = "1.1.1";

            src = self;

            buildInputs = with pkgs; [
              libsndfile
              libpng
              libjpeg-turbo
              alsa-lib
            ];

            buildPhase = ''
              runHook preBuild
              g++ main.cpp -o cmuspp -std=c++17 -O3 -Wall -Wextra -pthread \
                -DCMUSPP_HAS_JPEG -DCMUSPP_HAS_PNG \
                -lsndfile -lpng -ljpeg -lasound
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              mkdir -p $out/bin $out/share/cmuspp/themes $out/share/licenses/cmuspp
              cp cmuspp $out/bin/
              cp LICENSE $out/share/licenses/cmuspp/
              cp themes/*.xml $out/share/cmuspp/themes/
              runHook postInstall
            '';

            meta = with pkgs.lib; {
              description = "C++17 terminal music player with cover art, lyrics and themes";
              homepage = "https://github.com/Ars-byte/cmuspp";
              license = licenses.mit;
              platforms = platforms.linux;
              maintainers = [ ];
            };
          };
          default = self.packages.${system}.cmuspp;
        });

      apps = forAllSystems (system: {
        default = {
          type = "app";
          program = "${self.packages.${system}.cmuspp}/bin/cmuspp";
        };
      });
    };
}
