{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    llvmPackages_22.clang-tools
  ];
}
