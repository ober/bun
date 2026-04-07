import { spawnSync } from "child_process";
import fs from "fs";
import path from "path";
import { writeIfNotChanged } from "./helpers";

const input = process.argv[2];
const output = process.argv[3];

const platform = process.env.TARGET_PLATFORM ?? process.platform;

const create_hash_table = path.join(import.meta.dir, "./create_hash_table");

const input_text = fs.readFileSync(input, "utf8");
const to_preprocess = [...input_text.matchAll(/@begin\s+.+?@end/gs)].map(m => m[0]).join("\n");

const os = platform === "win32" ? "WINDOWS" : platform.toUpperCase();
const other_oses = ["WINDOWS", "DARWIN", "LINUX"].filter(x => x !== os);
const to_remove = new RegExp(`#if\\s+(!OS\\(${os}\\)|OS\\((${other_oses.join("|")})\\))\\n.*?#endif`, "gs");

const input_preprocessed = to_preprocess.replace(to_remove, "");

console.log("Generating " + output + " from " + input);
const result = spawnSync("perl", [create_hash_table, "-"], {
  input: input_preprocessed,
  stdio: ["pipe", "pipe", "inherit"],
  maxBuffer: 64 * 1024 * 1024,
});
if (result.status !== 0) {
  console.log(
    "Failed to generate " +
      output +
      ", create_hash_table exited with " +
      (result.status ?? "") +
      (result.signal ?? ""),
  );
  process.exit(1);
}
let str = typeof result.stdout === "string" ? result.stdout : result.stdout.toString("utf8");
str = str.replaceAll(/^\/\/.*$/gm, "");
str = str.replaceAll(/^#include.*$/gm, "");
str = str.replaceAll(`namespace JSC {`, "");
str = str.replaceAll(`} // namespace JSC`, "");
str = str.replaceAll(/NativeFunctionType,\s([a-zA-Z0-99_]+)/gm, "NativeFunctionType, &$1");
str = str.replaceAll("&Generated::", "Generated::");
str = "#pragma once" + "\n" + "// File generated via `create-hash-table.ts`\n" + str.trim() + "\n";

writeIfNotChanged(output, str);
