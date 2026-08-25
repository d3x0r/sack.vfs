/**
 * Types for the CommonJS shim (events.js), which assigns everything from
 * events.mjs onto module.exports at load time.  tsc cannot infer that from a
 * dynamic import, so the declarations are re-exported by hand -- keep this in
 * step with events.mjs if its exports change.
 */
export * from "./events.mjs";
