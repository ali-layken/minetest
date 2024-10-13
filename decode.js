const child_process = require("child_process");
const fs = require("fs");

const file = fs.readFileSync(process.argv[2], "utf8");
console.log(
  file.replace(
    /rtld:(0x[0-9a-f]{1,16})/g,
    (existing, offset) =>
      existing +
      " => " +
      child_process
        .execFileSync("C:/devkitPro/devkitA64/bin/aarch64-none-elf-addr2line.exe", [
          "-e",
          "./src/minetest.elf",
          "-f",
          "-p",
          "-C",
          "-a",
          offset,
        ])
        .toString("utf8")
        .trim()
  )
);
