{
  # options
  buildtype ? "release",
  compiler ? "clang",
  dep_type ? "shared",
  mesonFlags ? "",

  # deps
  system ? builtins.currentSystem,
  nixpkgs ? import <nixpkgs> { inherit system; }
}:

with nixpkgs;

stdenv.mkDerivation rec {
  name = "ffec";
  version = "0.1.1";
  outputs = [ "out" ];

  meta = with stdenv.lib; {
    description = "Fast FEC library";
    homepage = https://siriobalmelli.github.io/fastfec/;
    license = licenses.lgpl21Plus;
    platforms = platforms.unix;
    maintainers = [ "https://github.com/siriobalmelli" ];
  };

  nonlibc = nixpkgs.nonlibc or import ./nonlibc {};

  inputs = [
    nonlibc
    nixpkgs.gcc
    nixpkgs.clang
    nixpkgs.meson
    nixpkgs.ninja
    nixpkgs.pandoc
    nixpkgs.pkgconfig
  ];
  buildInputs = if ! lib.inNixShell then inputs else inputs ++ [
    nixpkgs.cscope
    nixpkgs.gdb
    nixpkgs.man
    nixpkgs.pahole
    nixpkgs.valgrind
    nixpkgs.which
  ];
  propagatedBuildInputs = [];

  # just work with the current directory (aka: Git repo), no fancy tarness
  src = ./.;

  # Override the setupHook in the meson nix derivation,
  # so that meson doesn't automatically get invoked from there.
  meson = pkgs.meson.overrideAttrs ( oldAttrs: rec { setupHook = ""; });

  # don't harden away position-dependent speedups for static builds
  hardeningDisable = [ "pic" "pie" ];

  # build
  mFlags = mesonFlags
    + " --buildtype=${buildtype}"
    + " -Ddep_type=${dep_type}";

  configurePhase = ''
      echo "flags: $mFlags"
      echo "prefix: $out"
      CC=${compiler} meson --prefix=$out build $mFlags
      cd build
  '';

  buildPhase = "ninja";
  doCheck = true;
  checkPhase = "ninja test";
  installPhase = "ninja install";
}
