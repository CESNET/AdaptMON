let
  pkgs = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/319e4cebef10cbb636c6ea392137b52ad105336a.tar.gz";
  }) {};
in
pkgs.mkShellNoCC {
  packages = [
    pkgs.llvmPackages_22.clang-tools
    pkgs.editorconfig-checker
  ];
}
