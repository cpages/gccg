with import <nixpkgs> {}; {
  gccgEnv = stdenv.mkDerivation {
    name = "gccg";
    buildInputs = [ stdenv SDL SDL_image SDL_net SDL_ttf SDL_mixer
      libjpeg ];
  };
}
