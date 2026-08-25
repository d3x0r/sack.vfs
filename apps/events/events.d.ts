/**
 * Types for the CommonJS shim (events.js), which assigns everything from
 * events.mjs onto module.exports at load time.
 *
 * HAND-WRITTEN -- do not generate this file. events.js has no static exports,
 * only a dynamic import whose result is Object.assign'd onto module.exports, so
 * `tsc events.js --declaration` emits an EMPTY declaration straight over this
 * one (or fails with TS5055 once this file is part of the program). makeTypes.bat
 * here lists only the .mjs sources for that reason.
 *
 * Keep in step with events.mjs if its exports change.
 */
export * from "./events.mjs";
