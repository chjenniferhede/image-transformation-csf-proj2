#! /usr/bin/env bash

# Run c_imgproc or asm_imgproc on test input and check whether
# the correct output images are produced.

error_count="0"

run_test() {
  local exe="./run_test.rb"
  echo -n "Running '${exe} $@'..."
  local out_stem=$(echo "$@" | tr ' ./' '___')
  local out_file="actual/${out_stem}.out"
  local err_file="actual/${out_stem}.err"
  ${exe} "$@" > ${out_file} 2> ${err_file}
  if [[ $? -ne 0 ]]; then
    echo "FAILED"
    error_count=$((${error_count} + 1))
  else
    echo "passed"
  fi
}

if [[ $# -ne 1 ]]; then
  >&2 echo "Usage: ./run_all <exe_version>"
  >&2 echo "  <exe_version> is either 'c' or 'asm'"
  exit 1
fi

exe_version="$1"

mkdir -p actual

image_stems='dice ingo kittens landscape'

for stem in ${image_stems}; do
  run_test ${exe_version} ${stem} squash 1 1
  run_test ${exe_version} ${stem} squash 2 3
  run_test ${exe_version} ${stem} squash 3 2

  run_test ${exe_version} ${stem} color_rot

  run_test ${exe_version} ${stem} blur 0
  run_test ${exe_version} ${stem} blur 5
  run_test ${exe_version} ${stem} blur 11

  run_test ${exe_version} ${stem} expand
done


if [[ ${error_count} -eq 0 ]]; then
  echo "All tests passed!"
  exit 0
else
  echo "${error_count} test(s) failed\n"
  exit 1
fi
