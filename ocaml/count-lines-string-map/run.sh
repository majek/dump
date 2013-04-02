#!/bin/bash
set -e
set -u
set -o pipefail

eval `opam config env`

echo "true:package(core),thread" > _tags
ocamlbuild -use-ocamlfind main.native counter.native


cat test01.in | ./main.native > test01.out-ocaml
cat test01.in | sort | uniq -c | sort -rn > test01.out-sort

diff test01.out-ocaml test01.out-sort || true

rm -rf *native test01.out-ocaml test01.out-sort _tags
