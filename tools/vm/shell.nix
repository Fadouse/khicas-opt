{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = with pkgs; [ gcc git meson ninja pkg-config python3 glib pixman ];
}
