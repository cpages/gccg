with import <nixpkgs> {}; {
  gccgEnv = stdenv.mkDerivation {
    name = "gccg";
    buildInputs = [ stdenv SDL2 SDL2_image SDL2_net SDL2_ttf SDL2_mixer
      libjpeg ];
  };
}
