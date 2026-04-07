/**
 * Platform shims — small dylibs/objects linked into the bun executable to
 * work around toolchain or OS bugs.
 *
 * Each shim is a ninja build edge (source → output), so ninja handles
 * rebuild-on-change. `emitShims()` registers the edges and returns the
 * linker flags + implicit inputs to spread into the final link() call.
 *
 * Every shim MUST have an entry in workarounds.ts that fails configure
 * once the upstream fix ships — see that file for the pattern.
 */

import { resolve } from "node:path";
import type { Config } from "./config.ts";
import type { Ninja } from "./ninja.ts";
import { quote } from "./shell.ts";
import { prebuiltDestDir } from "./deps/webkit.ts";

export interface ShimLinkOpts {
  /** Extra ldflags to append to the link() call. */
  ldflags: string[];
  /** Implicit inputs — ninja relinks if these change. */
  implicitInputs: string[];
}

const ASAN_DYLD_SHIM = "asan-dyld-shim.dylib";
const WTF_WEAKENED_STAMP = "wtf-crash-stub.stamp";

/**
 * Register shim compile rules. Call once from rules.ts alongside the
 * other registerXxxRules() calls.
 */
export function registerShimRules(n: Ninja, cfg: Config): void {
  const q = (p: string) => quote(p, false);

  if (cfg.darwin && cfg.asan) {
    // -install_name @rpath/<name> so dyld resolves it next to the
    // executable via the -rpath @executable_path we add at link time.
    // __DATA,__interpose only works from dylibs (not object files linked
    // into the main binary), hence -dynamiclib.
    n.rule("shim_dylib", {
      command: `${q(cfg.cc)} -dynamiclib -O2 -install_name @rpath/$name -o $out $in`,
      description: "shim $name",
    });
  }

  if (cfg.freebsd) {
    // Weaken WTFCrashWithInfoImpl in libWTF.a so the noop stub overrides it.
    // Derive llvm-objcopy path from the compiler path (same LLVM install).
    const objcopy = resolve(cfg.cc, "..", "llvm-objcopy");
    n.rule("weaken_wtf_crash", {
      command: `${q(objcopy)} --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_m --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_mm --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_mmm --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_mmmm --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_mmmmm --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_mmmmmm --weaken-symbol=_Z20WTFCrashWithInfoImpliPKcS0_mmmmmmm $in && echo done > $out`,
      description: "weaken WTFCrashWithInfoImpl in libWTF.a",
    });
  }
}

/**
 * Emit shim build edges and return link flags. Call once per link site
 * (emitBun, emitLinkOnly) before the link() call.
 *
 * Currently covers:
 * - macOS 26.4 ASAN dyld deadlock shim (see workarounds.ts)
 * - FreeBSD noop stub for WTFCrashWithInfoImpl (see wtf-crash-stub.c)
 */
export function emitShims(n: Ninja, cfg: Config): ShimLinkOpts {
  const ldflags: string[] = [];
  const implicitInputs: string[] = [];

  // ─── macOS ASAN dyld shim ───
  if (cfg.darwin && cfg.asan) {
    const src = resolve(cfg.cwd, "scripts", "build", "shims", "asan-dyld-shim.c");
    const out = resolve(cfg.buildDir, ASAN_DYLD_SHIM);

    n.build({
      outputs: [out],
      rule: "shim_dylib",
      inputs: [src],
      vars: { name: ASAN_DYLD_SHIM },
    });

    ldflags.push(out, "-Wl,-rpath,@executable_path");
    implicitInputs.push(out);
  }

  // ─── FreeBSD: weaken WTFCrashWithInfoImpl + noop stub ───
  if (cfg.freebsd) {
    // 1. Weaken WTFCrashWithInfoImpl symbols in libWTF.a
    const wtfLib = resolve(prebuiltDestDir(cfg), "lib", `${cfg.libPrefix}WTF${cfg.libSuffix}`);
    const stamp = resolve(cfg.buildDir, WTF_WEAKENED_STAMP);

    n.build({
      outputs: [stamp],
      rule: "weaken_wtf_crash",
      inputs: [wtfLib],
      // Re-run if WTF library changes (re-fetch)
      implicitInputs: [wtfLib],
      vars: { name: "libWTF.a" },
    });

    // 2. The stub object is compiled by the normal cxx rule (from glob-sources.ts).
    //    We just need it to appear BEFORE the WebKit libs in link order.
    //    bun.ts already puts obj/src/bun.js/bindings/wtf-crash-stub.c.o
    //    in the object list (it's a .c file, compiled via cc()).

    // 3. Add the stamp as an implicit input so ninja relinks when weakening runs.
    ldflags.push(stamp);
    implicitInputs.push(stamp);
  }

  return { ldflags, implicitInputs };
}
