// WebGPU binding generator — Stages 1 + 2a: parse, coverage dump, and
// per-member emit-classification (GEN / HAND / SKIP).
//
// Layout (paths relative to src/gui/webgpu/):
//   webgpu.idl   — W3C editor's draft (vendored)
//                  source: https://gpuweb.github.io/gpuweb/webgpu.idl
//   dawn.json    — Dawn pinned tag's API description (vendored)
//   generator/gen.mjs                     — this file
//   generated/webgpu_generated.manifest   — output: text summary, one line
//                                           per IDL member, tagged with dawn
//                                           match status
//
// This stage emits NO C++ — it only verifies that the IDL parses, that mixins
// resolve, and that IDL→dawn name mapping covers the surface we care about.
// Run with: node src/gui/webgpu/generator/gen.mjs
//
// Naming conventions discovered:
//   IDL interface `GPUDevice`      ↔ dawn key  `device`
//   IDL operation `createBuffer`   ↔ dawn name `create buffer`
//   IDL attribute `lost` (getter)  ↔ dawn name `get lost`        (heuristic)

import * as webidl2 from "webidl2";
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const HERE     = dirname( fileURLToPath( import.meta.url ) );  // .../webgpu/generator
const ROOT     = join( HERE, ".." );                            // .../webgpu
const OUT_DIR  = join( ROOT, "generated" );

// ---------- load inputs ----------

const idlText  = readFileSync( join( ROOT, "webgpu.idl" ),  "utf8" );
const dawnJson = JSON.parse( readFileSync( join( ROOT, "dawn.json" ), "utf8" ) );

const idl = webidl2.parse( idlText );

// ---------- index IDL ----------

const interfaces  = new Map(); // name -> node
const mixins      = new Map(); // name -> node
const dictionaries = new Map();
const enums       = new Map();
const typedefs    = new Map();
const namespaces  = new Map();
const includes    = []; // { target, includes }

// IDL allows `partial interface GPUDevice { ... }` and `partial dictionary X`
// to add members to a previously-declared type. webidl2 emits them as separate
// nodes; we merge their members into the canonical node.
function mergePartial( map, node ) {
	const existing = map.get( node.name );
	if( !existing ) { map.set( node.name, node ); return; }
	existing.members.push( ...node.members );
}

for( const node of idl ) {
	switch( node.type ) {
		case "interface":       mergePartial( interfaces, node );    break;
		case "interface mixin": mergePartial( mixins, node );        break;
		case "dictionary":      mergePartial( dictionaries, node );  break;
		case "enum":            enums.set( node.name, node );        break;
		case "typedef":         typedefs.set( node.name, node );     break;
		case "namespace":       namespaces.set( node.name, node );   break;
		case "includes":        includes.push( node );               break;
	}
}

// Resolve "Foo includes Bar" — fold mixin members into the target interface.
// We do not mutate the IDL nodes; we collect a member list per interface name.
const interfaceMembers = new Map();
for( const [ name, iface ] of interfaces )
	interfaceMembers.set( name, [ ...iface.members ] );

for( const inc of includes ) {
	const mixin = mixins.get( inc.includes );
	if( !mixin ) continue;
	const members = interfaceMembers.get( inc.target );
	if( !members ) continue;
	for( const m of mixin.members )
		members.push( m );
}

// ---------- index dawn ----------

const dawnObjects    = {}; // key -> { methods: Map(name -> entry) }
const dawnStructures = {}; // key -> { fields: Set(name), raw }
const dawnEnums      = new Set();
const dawnBitmasks   = new Set();

for( const [ key, entry ] of Object.entries( dawnJson ) ) {
	switch( entry.category ) {
		case "object": {
			const methods = new Map();
			for( const m of entry.methods || [] )
				methods.set( m.name, m );
			dawnObjects[ key ] = { methods, raw: entry };
			break;
		}
		case "structure": {
			// Map field name → { type, default } so emitters can confirm a
			// dawn field exists AND that its C type is compatible with what
			// we'd write into it.
			const fields = new Map();
			for( const f of entry.members || [] )
				fields.set( f.name, {
					type: f.type, default: f.default,
					annotation: f.annotation,   // "*" / "const*" for pointer fields
					length: f.length,           // count-field name for arrays
				} );
			dawnStructures[ key ] = { fields, raw: entry };
			break;
		}
		case "enum":      dawnEnums.add( key );      break;
		case "bitmask":   dawnBitmasks.add( key );   break;
	}
}

// ---------- IDL type classification ----------
//
// For each method/attribute/dict-field we need to know whether the generator
// can produce a C++ body for it, or whether it must be hand-written/skipped.
//
// classifyType( idlType ) returns one of:
//   { kind: "primitive", name, cType }
//   { kind: "enum",      name }       — wraps a WGPU<Enum>; uses string↔int table
//   { kind: "handle",    name }       — interface; passed by JS wrapper object
//   { kind: "dict",      name }       — dictionary; read from JS options bag
//   { kind: "sequence",  element }    — sequence<T> / FrozenArray<T>
//   { kind: "promise",   resolved }   — Promise<T>; async path
//   { kind: "union",     members }    — (A or B)
//   { kind: "callback",  name }       — JS function callback
//   { kind: "unknown",   name }       — anything not recognized
//
// Typedefs are resolved transparently — GPUSize64 → uint64_t etc.

const primitives = new Map( Object.entries( {
	"undefined":           { c: "void" },
	"boolean":             { c: "bool" },
	"byte":                { c: "int8_t" },
	"octet":               { c: "uint8_t" },
	"short":               { c: "int16_t" },
	"unsigned short":      { c: "uint16_t" },
	"long":                { c: "int32_t" },
	"unsigned long":       { c: "uint32_t" },
	"long long":           { c: "int64_t" },
	"unsigned long long":  { c: "uint64_t" },
	"float":               { c: "float" },
	"unrestricted float":  { c: "float" },
	"double":              { c: "double" },
	"unrestricted double": { c: "double" },
	"USVString":           { c: "WGPUStringView" },
	"DOMString":           { c: "WGPUStringView" },
	"ByteString":          { c: "WGPUStringView" },
	"object":              { c: "void*" },
	"any":                 { c: "void*" },
	"ArrayBuffer":         { c: "void*" /* + length elsewhere */ },
	"ArrayBufferView":     { c: "void*" },
	"BufferSource":        { c: "void*" },
} ) );

// Resolve a chain of typedefs to its ultimate type node.
function resolveTypedef( t ) {
	while( t && typeof t.idlType === "string" && typedefs.has( t.idlType ) ) {
		const td = typedefs.get( t.idlType );
		// typedef's RHS is another idlType node
		t = td.idlType;
	}
	return t;
}

function classifyType( t ) {
	if( !t ) return { kind: "unknown", name: "?" };
	if( t.union ) {
		return { kind: "union", members: t.idlType.map( classifyType ) };
	}
	if( t.generic === "sequence" || t.generic === "FrozenArray" ) {
		const inner = Array.isArray( t.idlType ) ? t.idlType[ 0 ] : t.idlType;
		return { kind: "sequence", element: classifyType( inner ) };
	}
	if( t.generic === "Promise" ) {
		const inner = Array.isArray( t.idlType ) ? t.idlType[ 0 ] : t.idlType;
		return { kind: "promise", resolved: classifyType( inner ) };
	}
	if( t.generic === "record" ) {
		return { kind: "unknown", name: "record<...>" };
	}
	t = resolveTypedef( t );
	const name = typeof t.idlType === "string" ? t.idlType : "?";
	if( primitives.has( name ) ) {
		return { kind: "primitive", name, cType: primitives.get( name ).c };
	}
	if( enums.has( name ) )        return { kind: "enum",     name };
	if( interfaces.has( name ) )   return { kind: "handle",   name };
	if( dictionaries.has( name ) ) return { kind: "dict",     name };
	// Callbacks (e.g. GPUUncapturedErrorCallback) appear as named callback types
	// in some WebGPU specs. We don't index them yet — treat as unknown.
	return { kind: "unknown", name };
}

// Primitive type names that need special handling on the binding side and
// are therefore NOT emittable by the rote path. The buffer-y ones need
// zero-copy aliasing or source-pointer extraction; `any`/`object` are
// catch-alls we can't reason about.
const nonRotePrimitives = new Set( [
	"any", "object",
	"ArrayBuffer", "ArrayBufferView", "BufferSource",
] );

// Populated by classifyAllDicts() below — dict names for which a reader
// struct will be emitted. canEmitArg consults this for dict-typed args.
const genDicts = new Set();

// Can the generator emit this argument? (primitive, enum, handle, gen-dict,
// sequence-of-{primitive,enum,handle}.)
function canEmitArg( c ) {
	switch( c.kind ) {
		case "primitive":  return !nonRotePrimitives.has( c.name );
		case "enum":       return true;
		case "handle":     return true;
		case "dict":       return genDicts.has( c.name );
		case "sequence":   {
			const e = c.element;
			if( e.kind === "handle" )    return true;
			if( e.kind === "primitive" ) return !nonRotePrimitives.has( e.name );
			if( e.kind === "enum" )      return true;
			// sequence-of-dict needs placement-new + per-element holder
			// lifetimes; out of scope for this stage.
			return false;
		}
		default:           return false;
	}
}

function canEmitReturn( c ) {
	switch( c.kind ) {
		case "primitive":  return !nonRotePrimitives.has( c.name );
		case "enum":       return true;
		case "handle":     return true;
		default:           return false;
	}
}

// Decide GEN / HAND / SKIP for an operation. Returns a string tag + reason.
function classifyOperation( opNode, dawnObj, dawnMethod ) {
	if( !dawnObj )      return { tag: "SKIP", reason: "no dawn object" };
	if( !dawnMethod )   return { tag: "SKIP", reason: "no dawn method" };

	const args = ( opNode.arguments || [] ).map( a => ({
		name: a.name,
		optional: !!a.optional,
		// webidl2 default shape: {type: "number"|"string"|"boolean"|...,
		// value: "1"|...}. We pass through whatever's there; emitArgRead
		// translates per primitive kind. Critical for draw/dispatch where
		// instanceCount defaults to 1 (not 0).
		default: a.default || null,
		// idlName: original IDL type name before typedef resolution.
		// Lets emitArgRead detect domain-specific aliases (e.g. GPUSize64
		// optional with no default → WGPU_WHOLE_SIZE, not 0).
		idlName: typeof a.idlType?.idlType === "string" ? a.idlType.idlType : null,
		type: classifyType( a.idlType ),
	}) );
	const ret = classifyType( opNode.idlType );

	// Promise<T>: defer to hand-written async path for now.
	if( ret.kind === "promise" ) {
		return { tag: "HAND", reason: "async (Promise return)", args, ret };
	}

	// Any non-emittable arg pushes to HAND/SKIP territory.
	for( const a of args ) {
		if( !canEmitArg( a.type ) ) {
			return { tag: "HAND", reason: `arg "${a.name}" is ${a.type.kind}${a.type.name ? ":" + a.type.name : ""}`, args, ret };
		}
	}
	if( !canEmitReturn( ret ) ) {
		return { tag: "HAND", reason: `return is ${ret.kind}${ret.name ? ":" + ret.name : ""}`, args, ret };
	}

	return { tag: "GEN", reason: "ok", args, ret };
}

// Walk the IDL inheritance chain to collect all fields a dict has, including
// fields inherited from base dicts (e.g. `label` from GPUObjectDescriptorBase).
// Returns [{name, required, type, fromBase}], child-overrides-base by name.
function collectDictFields( dictNode ) {
	const chain = [];
	let cur = dictNode;
	while( cur ) {
		chain.push( cur );
		cur = cur.inheritance ? dictionaries.get( cur.inheritance ) : null;
	}
	// Bases first so children can override.
	const byName = new Map();
	for( let i = chain.length - 1; i >= 0; i-- ) {
		const node = chain[ i ];
		const fromBase = ( node !== dictNode );
		for( const m of node.members ) {
			byName.set( m.name, {
				name: m.name,
				required: !!m.required,
				type: classifyType( m.idlType ),
				fromBase,
				default: m.default,
			} );
		}
	}
	return [ ...byName.values() ];
}

// Same for a dictionary — can the generator emit a reader for it?
// Includes inherited fields. Dict fields are slightly more capable than
// method args: they can host sequence-of-dict via the per-reader RAII
// pattern (allocate readers + element-desc array, free in dtor).
function isSeqOfGenDict( t ) {
	return t.kind === "sequence"
	    && t.element.kind === "dict"
	    && genDicts.has( t.element.name );
}
function classifyDict( dictNode ) {
	// Lenient: emit a reader whenever there's a dawn struct, even if some
	// fields can't be expressed (unions, chained-struct sources, sequences
	// of inlined-base types, ...). The emitter drops a TODO comment for
	// each unhandlable field; downstream JS callers can either avoid those
	// fields or hand-wrap the method that uses the dict.
	const fields = collectDictFields( dictNode );
	const skipped = [];
	for( const f of fields ) {
		if( !canEmitArg( f.type ) && !isSeqOfGenDict( f.type ) )
			skipped.push( `${f.name}:${f.type.kind}${f.type.name ? ":" + f.type.name : ""}` );
	}
	return {
		tag: "GEN",
		reason: skipped.length ? `partial (skipped: ${skipped.join( ", " )})` : "ok",
		fields,
	};
}

// Pre-pass: decide which dicts are GEN-able and populate genDicts so that
// canEmitArg() can consult it during operation classification. Run twice —
// once with empty genDicts (no dicts can be nested args, so we get the floor),
// then a fixpoint iteration for dicts that nest other GEN dicts.
function classifyAllDicts() {
	for( let pass = 0; pass < 3; pass++ ) {
		const before = genDicts.size;
		for( const [ name, dict ] of dictionaries ) {
			if( ( name in interfaceOverrides ) && interfaceOverrides[ name ] === "" ) continue;
			if( inlinedBaseDicts.has( name ) ) continue;
			const dawnKey = idlInterfaceToDawn( name );
			if( !( dawnKey && dawnKey in dawnStructures ) ) continue;
			const c = classifyDict( dict );
			if( c.tag === "GEN" ) genDicts.add( name );
		}
		if( genDicts.size === before ) break;
	}
}
// Invocation deferred to just before the manifest walk — it needs the name
// mapping helpers defined further below.

// ---------- name mapping ----------

// Methods that are hand-written in webgpu_module.cc and must NOT be wired
// by the generator. V8's ObjectTemplate::Set with a duplicate key doesn't
// reliably override prior bindings (behavior varies across versions), so we
// avoid the conflict by skipping these in the generated wireGenerated_<X>
// functions entirely. The hand-written Init for the class registers them.
const handWrittenMethods = new Set( [
	"GPUDevice.createShaderModule",     // chained WGSL source struct
	"GPUDevice.createRenderPipeline",   // union layout
	"GPUDevice.createComputePipeline",  // union layout
	"GPUDevice.createTexture",          // GPUExtent3D union (sequence|dict)
	"GPUDevice.createBindGroup",        // resource union per entry
	"GPUDevice.popErrorScope",          // async Promise<GPUError?>
	"GPUQueue.writeBuffer",             // raw buffer source
	"GPUQueue.writeTexture",            // raw buffer source + extent union
	"GPUBuffer.mapAsync",               // async Promise + Dawn callback
	"GPUBuffer.getMappedRange",         // zero-copy ArrayBuffer aliasing
	"GPUBuffer.unmap",                  // detaches the ArrayBuffer
	"GPUCommandEncoder.beginRenderPass",// sequence-of-dict color attachments
	"GPUCommandEncoder.finish",         // hand: release encoder handle (spec-invalidated)
	"GPURenderPassEncoder.end",         // hand: release pass handle (spec-invalidated)
	"GPUTexture.createView",            // hand: stash sourceTexture_ on the view (auto-present path)
	"GPUQueue.submit",                  // hand: auto-present if a submitted CB wrote to the surface
	"GPUBuffer.destroy",                // hand: release Dawn handle (spec-invalidated by destroy)
	"GPUTexture.destroy",               // hand: release Dawn handle (spec-invalidated by destroy)
	"GPURenderPassEncoder.draw",        // hand: per-frame draw tally
	"GPURenderPassEncoder.drawIndexed", // hand: per-frame draw tally
] );

// Hand-coded overrides for cases the regex can't reach. Empty string = "skip,
// this IDL type has no dawn equivalent and will be hand-written or omitted".
const interfaceOverrides = {
	GPU:                    "instance",                          // navigator.gpu ↔ WGPUInstance
	GPUSupportedLimits:     "limits",                            // IDL adds "Supported" prefix
	WGSLLanguageFeatures:   "supported WGSL language features",  // reordered
	// Hand-written, not generated:
	GPUCanvasContext:       "",   // browser concept; we wire WGPUSurface manually
	GPUDeviceLostInfo:      "",   // delivered via device-lost callback args
	GPUError:               "",   // base class; concrete errors are JS-synthesized
	GPUValidationError:     "",
	GPUOutOfMemoryError:    "",
	GPUInternalError:       "",
	GPUPipelineError:       "",
	GPUUncapturedErrorEvent:"",
};

// CamelCase → dawn's spaced-lowercase convention, with two preservations:
//  (a) split between letter and digit:  dimension1D → dimension 1D
//  (b) trailing single capital after a digit stays uppercase:  3D, 2D, 1D
function camelToDawn( s ) {
	s = s.replace( /([a-z0-9])([A-Z])/g, "$1 $2" )         // lower-or-digit → Upper
	     .replace( /([A-Z]+)([A-Z][a-z])/g, "$1 $2" )      // ABCdef → AB Cdef
	     .replace( /([a-zA-Z])(\d)/g, "$1 $2" );           // letter → digit
	s = s.toLowerCase();
	// Restore "1D"/"2D"/"3D" tokens: pattern "<digit> <single-letter>" at word boundary
	s = s.replace( /(\d) ([a-z])(?=\s|$)/g, ( _, d, c ) => d + c.toUpperCase() );
	return s;
}

// IDL interface → dawn key. Strips GPU/Dict; honors overrides.
function idlInterfaceToDawn( name ) {
	if( name in interfaceOverrides ) return interfaceOverrides[ name ];
	let s = name.startsWith( "GPU" ) ? name.slice( 3 ) : name;
	if( s.endsWith( "Dict" ) ) s = s.slice( 0, -4 );        // GPUColorDict → Color
	return camelToDawn( s );
}

// createBuffer → "create buffer"; getCurrentTexture → "get current texture"
function idlOpToDawn( name ) { return camelToDawn( name ); }

// IDL attribute `foo` → dawn method `"get foo"` (object case) or struct field `"foo"`
function idlAttrToDawnGetter( name ) { return "get " + camelToDawn( name ); }
function idlAttrToDawnField ( name ) { return camelToDawn( name ); }

// Attributes that appear on every WebGPU object and don't follow the get/set
// convention. Each entry says how to emit the binding.
const universalAttributes = {
	// IDL `label` is stored on the JS wrapper; setter calls wgpu<Type>SetLabel(obj, str).
	label: { route: "wrapper-stored + wgpu<Type>SetLabel" },
};

// Dictionaries that exist only as IDL inheritance bases or mixed-in fragments —
// dawn does not give them a struct, their fields are inlined into each concrete
// descriptor instead. The generator should NOT emit a marshaler for these;
// instead, when emitting a concrete descriptor that inherits from one, walk
// the IDL chain and inline the parent's fields.
const inlinedBaseDicts = new Set( [
	"GPUObjectDescriptorBase",            // adds `label` to every descriptor
	"GPUPipelineDescriptorBase",          // adds `layout` to pipeline descriptors
	"GPUProgrammableStage",               // module + entryPoint + constants (mixed into stage dicts)
	"GPUBufferBinding",                   // {buffer,offset,size} — appears inline in bind group entries
	"GPUShaderModuleCompilationHint",     // appears as inline array element
	"GPUComputePassTimestampWrites",      // dawn flattens into compute pass descriptor
	"GPURenderPassTimestampWrites",       // dawn flattens into render pass descriptor
	"GPURenderPassLayout",                // dawn flattens into render bundle encoder descriptor
] );

// ---------- walk + classify ----------

const manifestLines = [];
const counts = {
	interfaces: 0, ifaceMatched: 0,
	operations: 0, opMatched: 0,
	attributes: 0, attrMatched: 0,
	dictionaries: 0, dictMatched: 0,
	enums: 0, enumMatched: 0,
	// Emit classification — counted across operations/attributes/dicts that
	// also matched (so GEN+HAND+SKIP ≤ matched totals).
	opGen: 0, opHand: 0, opSkip: 0,
	dictGen: 0, dictHand: 0,
};
const unmatched = { interfaces: [], operations: [], attributes: [], dictionaries: [], enums: [] };
const handReasons = [];   // free-text "HAND: <why>" lines, capped when reported

// Now that name mapping is set up, run the dict pre-pass so canEmitArg() can
// route dict-typed args through the GEN path during the walk below.
classifyAllDicts();

manifestLines.push( "# WebGPU binding manifest" );
manifestLines.push( "# Generated by gen.mjs — do not edit." );
manifestLines.push( "" );

// Interfaces — try dawn object first, then dawn structure (read-only data carrier).
// Empty-string override = intentionally not generated (canvas, errors, etc).
manifestLines.push( "## Interfaces" );
for( const [ name, iface ] of interfaces ) {
	counts.interfaces++;
	const dawnKey = idlInterfaceToDawn( name );
	const skipped = ( name in interfaceOverrides ) && interfaceOverrides[ name ] === "";
	const dawnObj = dawnKey ? dawnObjects[ dawnKey ] : null;
	const dawnStr = dawnKey && !dawnObj ? dawnStructures[ dawnKey ] : null;

	let tag, kind;
	if( skipped )      { tag = "SKIP"; kind = "(hand-written)"; }
	else if( dawnObj ) { tag = "OK  "; kind = `object:${dawnKey}`; counts.ifaceMatched++; }
	else if( dawnStr ) { tag = "OK*"; kind = `struct:${dawnKey}`; counts.ifaceMatched++; }
	else               { tag = "MISS"; kind = `?${dawnKey}`; unmatched.interfaces.push( `${name} → ${dawnKey}` ); }

	manifestLines.push( `[${tag}] interface ${name}  ↔  dawn:${kind}` );
	if( skipped ) continue;

	const members = interfaceMembers.get( name ) || [];
	for( const m of members ) {
		if( m.type === "operation" ) {
			counts.operations++;
			const opDawn = idlOpToDawn( m.name );
			const hit = dawnObj && dawnObj.methods.has( opDawn );
			if( hit ) counts.opMatched++;
			else unmatched.operations.push( `${name}.${m.name} → ${dawnKey}."${opDawn}"` );
			// Emit classification (only for matched methods on dawn objects).
			let emitTag = "    ";  // blank slot for unmatched ops
			if( hit ) {
				const c = classifyOperation( m, dawnObj, dawnObj.methods.get( opDawn ) );
				emitTag = c.tag;
				if( c.tag === "GEN" )       counts.opGen++;
				else if( c.tag === "HAND" ) { counts.opHand++; handReasons.push( `${name}.${m.name}: ${c.reason}` ); }
				else if( c.tag === "SKIP" ) counts.opSkip++;
			}
			manifestLines.push( `    [${hit ? "OK  " : "MISS"}][${emitTag}] op ${m.name}  ↔  "${opDawn}"` );
		} else if( m.type === "attribute" ) {
			counts.attributes++;
			let hit = false, route = "", tag = "MISS";
			if( m.name in universalAttributes ) {
				hit = true;
				tag  = "UNIV";
				route = universalAttributes[ m.name ].route;
			} else if( dawnObj ) {
				const k = idlAttrToDawnGetter( m.name );
				if( dawnObj.methods.has( k ) ) { hit = true; route = `method:"${k}"`; }
				else route = `method:"${k}"`;
			} else if( dawnStr ) {
				const k = idlAttrToDawnField( m.name );
				if( dawnStr.fields.has( k ) ) { hit = true; route = `field:"${k}"`; }
				else route = `field:"${k}"`;
			} else {
				route = "(no dawn target)";
			}
			if( hit ) { counts.attrMatched++; if( tag === "MISS" ) tag = "OK  "; }
			else unmatched.attributes.push( `${name}.${m.name} → ${dawnKey} ${route}` );
			manifestLines.push( `    [${tag}] attr ${m.name}${m.readonly ? " (ro)" : ""}  ↔  ${route}` );
		} else if( m.type === "const" ) {
			manifestLines.push( `    [----] const ${m.name} = ${m.value && m.value.value}` );
		}
	}
}
manifestLines.push( "" );

// Dictionaries → dawn structures
manifestLines.push( "## Dictionaries" );
for( const [ name, dict ] of dictionaries ) {
	counts.dictionaries++;
	const dawnKey = idlInterfaceToDawn( name );
	const inlined = inlinedBaseDicts.has( name );
	const hit     = !inlined && dawnKey && ( dawnKey in dawnStructures );
	let tag;
	if( inlined ) { tag = "INLN"; counts.dictMatched++; }
	else if( hit ) { tag = "OK  "; counts.dictMatched++; }
	else { tag = "MISS"; unmatched.dictionaries.push( `${name} → ${dawnKey || "(skipped)"}` ); }

	let emitTag = "    ";
	if( hit ) {
		const c = classifyDict( dict );
		emitTag = c.tag;
		if( c.tag === "GEN" )  counts.dictGen++;
		else                   { counts.dictHand++; handReasons.push( `${name}: ${c.reason}` ); }
	}

	const inh = dict.inheritance ? ` : ${dict.inheritance}` : "";
	manifestLines.push( `[${tag}][${emitTag}] dict ${name}${inh}  ↔  dawn struct:${dawnKey}  (${dict.members.length} fields)` );
}
manifestLines.push( "" );

// Enums → dawn enums or bitmasks
manifestLines.push( "## Enums" );
for( const [ name, en ] of enums ) {
	counts.enums++;
	const dawnKey = idlInterfaceToDawn( name );
	const hit = dawnEnums.has( dawnKey ) || dawnBitmasks.has( dawnKey );
	if( hit ) counts.enumMatched++;
	else unmatched.enums.push( `${name} → ${dawnKey}` );
	manifestLines.push( `[${hit ? "OK  " : "MISS"}] enum ${name}  ↔  dawn:${dawnKey}  (${en.values.length} values)` );
}
manifestLines.push( "" );

// Mixins (informational — folded into interfaces above)
manifestLines.push( "## Mixins (folded into interfaces above)" );
for( const [ name, mx ] of mixins )
	manifestLines.push( `  ${name}  (${mx.members.length} members)` );
manifestLines.push( "" );

// ---------- emit C++ for GEN-eligible methods ----------
//
// Per-class structure of generated output:
//   - One `static void gen_<Class>_<method>(...)` per GEN operation
//   - One `void wireGenerated_<Class>(Isolate*, Local<FunctionTemplate>)`
//     that registers all of the above on the prototype template
//
// Hand-written class code calls wireGenerated_<Class> from its Init() to
// pick up the generated method table. Classes whose C++ wrapper hasn't been
// written yet are guarded by `#ifdef WGPU_HAVE_<Class>`; the user defines
// the guard when they declare the wrapper.
//
// Naming conventions:
//   JS class:  GPUBuffer        →  WGPU type:  WGPUBuffer
//   JS method: getMappedRange   →  C symbol:   wgpuBufferGetMappedRange
//
//   wgpu<Class-no-GPU><Method-PascalCase>(handle, args...)
//   The dawn.json method names ("get mapped range") drop spaces and
//   PascalCase each word. We compute it from the IDL method name +
//   the dawn-tag class name.

function pascal( s )  { return s.charAt( 0 ).toUpperCase() + s.slice( 1 ); }
function cClassFromDawn( dawnKey ) {
	// "command encoder" → "CommandEncoder"
	return dawnKey.split( /\s+/ ).map( pascal ).join( "" );
}
function cMethodFromDawn( dawnName ) {
	return dawnName.split( /\s+/ ).map( pascal ).join( "" );
}
function wgpuFn( dawnClass, dawnMethod ) {
	return "wgpu" + cClassFromDawn( dawnClass ) + cMethodFromDawn( dawnMethod );
}

// Map a dawn-side struct member name ("max texture dimension 1D",
// "mapped at creation") to the C struct field name. Dawn's C struct fields
// follow camelCase: spaces removed, words after the first PascalCased,
// trailing 1D/2D/3D preserved.
function dawnFieldToC( name ) {
	const parts = name.split( /\s+/ );
	return parts.map( ( p, i ) => i === 0
		? p.charAt( 0 ).toLowerCase() + p.slice( 1 )
		: p.charAt( 0 ).toUpperCase() + p.slice( 1 ) ).join( "" );
}

// Map our IDL field name (camelCase) to its dawn struct member by trying the
// same camelToDawn transform; the C field name is then dawnFieldToC of that.
function idlFieldToCField( idlName ) {
	return dawnFieldToC( camelToDawn( idlName ) );
}

// Emit a V8 read for one arg into a local C value.
// Returns { decls, errors, value } — caller stitches together.
function emitArgRead( i, arg, cls ) {
	const t = arg.type;
	const local = "_arg" + i;

	// Translate the webidl2 default node to a C literal for the matching
	// primitive kind. Falls back to "0"/"false" when no default is given.
	function defaultLiteral( kind ) {
		const d = arg.default;
		if( !d ) return kind === "bool" ? "false" : "0";
		if( d.type === "number" )  return String( d.value );
		if( d.type === "boolean" ) return d.value ? "true" : "false";
		if( d.type === "null" )    return "0";
		return kind === "bool" ? "false" : "0";
	}

	if( t.kind === "primitive" ) {
		switch( t.cType ) {
			case "bool": {
				const dflt = defaultLiteral( "bool" );
				return {
					decls: `\tbool ${local} = args.Length() > ${i} ? args[${i}]->BooleanValue( isolate ) : ${dflt};`,
					value: local,
				};
			}
			case "int32_t":
			case "uint32_t":
			case "int16_t":
			case "uint16_t":
			case "int8_t":
			case "uint8_t": {
				const dflt = defaultLiteral( "int" );
				return {
					decls: `\t${t.cType} ${local} = args.Length() > ${i} ? (${t.cType})args[${i}]->Int32Value( context ).FromMaybe( ${dflt} ) : (${t.cType})${dflt};`,
					value: local,
				};
			}
			case "int64_t":
			case "uint64_t": {
				// Optional GPUSize64 with no IDL default means "rest of
				// buffer" per WebGPU spec — Dawn's sentinel is WGPU_WHOLE_SIZE.
				// (Used by setVertexBuffer/setIndexBuffer size, mapAsync size,
				// copyBufferToBuffer size, etc.)
				const dflt = ( arg.optional && !arg.default
				               && arg.idlName === "GPUSize64" )
					? "WGPU_WHOLE_SIZE"
					: defaultLiteral( "int" );
				return {
					decls: `\t${t.cType} ${local} = args.Length() > ${i} ? (${t.cType})args[${i}]->IntegerValue( context ).FromMaybe( ${dflt} ) : (${t.cType})${dflt};`,
					value: local,
				};
			}
			case "float":
			case "double": {
				const dflt = defaultLiteral( "float" );
				return {
					decls: `\t${t.cType} ${local} = args.Length() > ${i} ? (${t.cType})args[${i}]->NumberValue( context ).FromMaybe( ${dflt} ) : (${t.cType})${dflt};`,
					value: local,
				};
			}
			case "WGPUStringView":
				return {
					decls:
`\tWGPU_StringViewHolder ${local}_h( isolate, args[${i}] );
\tWGPUStringView ${local} = ${local}_h.view();`,
					value: local,
				};
		}
		return { decls: `\t/* TODO arg ${i}: primitive ${t.name} */`, value: "0" };
	}
	if( t.kind === "enum" ) {
		const wgpuT = "WGPU" + t.name.replace( /^GPU/, "" );
		if( genEnums.has( t.name ) ) {
			return {
				decls:
`\tv8::String::Utf8Value ${local}_str( isolate, args[${i}] );
\t${wgpuT} ${local} = ( args.Length() > ${i} && args[${i}]->IsString() )
\t\t? wgpu_str_to_${t.name}( *${local}_str, (size_t)${local}_str.length(), (${wgpuT})0 )
\t\t: (${wgpuT})0;`,
				value: local,
			};
		}
		return {
			decls: `\t${wgpuT} ${local} = (${wgpuT})0; /* TODO: enum ${t.name} not in dawn pin */`,
			value: local,
		};
	}
	if( t.kind === "handle" ) {
		const wrapper = t.name; // e.g. GPUBuffer
		const wgpuT = "WGPU" + wrapper.replace( /^GPU/, "" );
		return {
			decls:
`\t${wgpuT} ${local} = NULL;
\tif( args.Length() > ${i} && args[${i}]->IsObject() ) {
\t\t${wrapper}* w = node::ObjectWrap::Unwrap<${wrapper}>( args[${i}].As<v8::Object>() );
\t\t${local} = w->handle_;
\t}`,
			value: local,
		};
	}
	if( t.kind === "dict" && genDicts.has( t.name ) ) {
		// Stack-allocate a reader struct; its constructor walks the JS opts
		// bag into the WGPU descriptor and owns lifetimes of any string views.
		return {
			decls:
`\tv8::Local<v8::Object> ${local}_opts = ( args.Length() > ${i} && args[${i}]->IsObject() )
\t\t? args[${i}].As<v8::Object>() : v8::Object::New( isolate );
\twgpu_read_${t.name} ${local}_r( isolate, context, ${local}_opts );`,
			value: `&${local}_r.desc`,
		};
	}
	if( t.kind === "sequence" ) {
		const e = t.element;
		// Element C type, JS-side extraction, and per-element assignment.
		let cT, perElem;
		if( e.kind === "handle" ) {
			cT = "WGPU" + e.name.replace( /^GPU/, "" );
			perElem = `\t\t\tif( _v->IsObject() ) {
\t\t\t\t${e.name}* _w = node::ObjectWrap::Unwrap<${e.name}>( _v.As<v8::Object>() );
\t\t\t\t${local}[_i] = _w->handle_;
\t\t\t}`;
		} else if( e.kind === "primitive" ) {
			cT = e.cType;
			if( /^u?int(8|16|32)_t$/.test( cT ) )
				perElem = `\t\t\t${local}[_i] = (${cT})_v->Int32Value( context ).FromMaybe( 0 );`;
			else if( /^u?int64_t$/.test( cT ) )
				perElem = `\t\t\t${local}[_i] = (${cT})_v->IntegerValue( context ).FromMaybe( 0 );`;
			else
				perElem = `\t\t\t${local}[_i] = (${cT})_v->NumberValue( context ).FromMaybe( 0 );`;
		} else /* enum */ {
			cT = "WGPU" + e.name.replace( /^GPU/, "" );
			perElem = `\t\t\tv8::String::Utf8Value _s( isolate, _v );
\t\t\t${local}[_i] = wgpu_str_to_${e.name}( *_s, (size_t)_s.length(), (${cT})0 );`;
		}
		return {
			decls:
`\tv8::Local<v8::Array> ${local}_arr = ( args.Length() > ${i} && args[${i}]->IsArray() )
\t\t? args[${i}].As<v8::Array>() : v8::Array::New( isolate, 0 );
\tsize_t ${local}_count = (size_t)${local}_arr->Length();
\t${cT}* ${local} = ${local}_count ? new ${cT}[${local}_count]() : NULL;
\tfor( size_t _i = 0; _i < ${local}_count; ++_i ) {
\t\tv8::Local<v8::Value> _v = ${local}_arr->Get( context, (uint32_t)_i ).ToLocalChecked();
${perElem}
\t}`,
			value: `${local}_count, ${local}`,
			after: `\tif( ${local} ) delete[] ${local};`,
		};
	}
	return { decls: `\t/* TODO arg ${i}: ${t.kind} */`, value: "0" };
}

// Emit the reader struct for one GEN-able dict.
//   struct wgpu_read_<Name> {
//       WGPU<Name> desc;
//       WGPU_StringViewHolder <field>_h;  // one per string field
//       wgpu_read_<Name>(isolate, context, opts);
//   };
// Wrapper classes the reader struct needs visible at compile time. Same idea
// as methodDeps: any handle field (or seq-of-handle) requires its wrapper.
function dictDeps( fields ) {
	const deps = new Set();
	for( const f of fields ) {
		if( f.type.kind === "handle" ) deps.add( f.type.name );
		else if( f.type.kind === "sequence" && f.type.element.kind === "handle" )
			deps.add( f.type.element.name );
	}
	return [ ...deps ];
}

// Type-compat between an IDL primitive field and a dawn struct member's
// type string. IDL and dawn occasionally disagree (xrCompatible isn't in the
// pinned dawn; featureLevel is enum in dawn but DOMString in IDL;
// depthWriteEnabled is WGPUOptionalBool in dawn but boolean in IDL).
function dawnFieldCompatibleWithPrimitive( dawnType, idlPrimName, cType ) {
	if( !dawnType ) return false;
	if( idlPrimName === "USVString" || idlPrimName === "DOMString"
	    || idlPrimName === "ByteString" )
		return dawnType === "string view";
	if( cType === "bool" )
		return dawnType === "bool" || dawnType === "optional bool";
	if( /^u?int(8|16|32)_t$/.test( cType ) || /^u?int64_t$/.test( cType ) )
		// Any plain integer-y dawn type, or any dawn bitmask (those are
		// uint64 under the hood and JS code ORs the constants).
		return /^(u?int(8|16|32|64)_t|size_t)$/.test( dawnType )
		    || dawnBitmasks.has( dawnType );
	if( cType === "float" || cType === "double" )
		return dawnType === "float" || dawnType === "double";
	return false;
}

function emitDictReader( name, fields ) {
	// Strip both "GPU" prefix and "Dict" suffix to get the C-side root:
	//   GPUBufferDescriptor → BufferDescriptor → WGPUBufferDescriptor
	//   GPUColorDict         → Color           → WGPUColor
	//   GPUOrigin3DDict      → Origin3D        → WGPUOrigin3D
	let cRoot = name.startsWith( "GPU" ) ? name.slice( 3 ) : name;
	if( cRoot.endsWith( "Dict" ) ) cRoot = cRoot.slice( 0, -4 );
	const wgpuT = "WGPU" + cRoot;
	const initMacro = "WGPU_" + camelToDawn( cRoot )
		.toUpperCase().replace( /\s+/g, "_" ) + "_INIT";

	const dawnKey = idlInterfaceToDawn( name );
	const dawnStr = dawnKey ? dawnStructures[ dawnKey ] : null;
	const dawnMembers = dawnStr ? dawnStr.fields : new Map();

	// Decide per-field what to emit. Cross-checks every IDL field against
	// the dawn struct member list; fields absent (or with an incompatible C
	// type) are skipped with a TODO so the .cc compiles.
	const emits = fields.map( f => {
		const dawnName = camelToDawn( f.name );
		const dawnEntry = dawnMembers.get( dawnName );
		if( !dawnEntry )
			return { f, kind: "skip", reason: `not in current dawn ${dawnKey || "struct"}` };
		const dawnType = dawnEntry.type;
		const cField = dawnFieldToC( dawnName );

		if( f.type.kind === "primitive" ) {
			const c = f.type.cType;
			if( !dawnFieldCompatibleWithPrimitive( dawnType, f.type.name, c ) )
				return { f, kind: "skip", reason: `IDL ${f.type.name} but dawn type "${dawnType}"` };
			if( c === "WGPUStringView" )           return { f, cField, kind: "string" };
			if( c === "bool" )                     return { f, cField,
				kind: dawnType === "optional bool" ? "optional-bool" : "bool" };
			if( c === "float" || c === "double" )  return { f, cField, kind: "float", c };
			if( /^u?int64_t$/.test( c ) )          return { f, cField, kind: "i64", c };
			return                                  { f, cField, kind: "i32", c };
		}
		if( f.type.kind === "enum" )
			return genEnums.has( f.type.name )
				? { f, cField, kind: "enum", enumName: f.type.name }
				: { f, cField, kind: "enum-todo" };
		if( f.type.kind === "handle" ) return { f, cField, kind: "handle" };
		if( f.type.kind === "dict" )
			return genDicts.has( f.type.name )
				? { f, cField, kind: "dict", dictName: f.type.name,
				    // Dawn annotates pointer-to-struct fields as "*"
				    // (non-const) or "const*" (const ptr).
				    isPointer: /\*/.test( dawnEntry.annotation || "" ) }
				: { f, cField, kind: "dict-todo" };
		if( f.type.kind === "sequence" ) {
			// Dawn models sequence-typed fields as a `(countField, arrayField)`
			// pair. The IDL field corresponds to the array; the count field
			// name comes from dawn's `length` annotation.
			if( !dawnEntry.length ) return { f, kind: "skip", reason: "dawn doesn't model as array" };
			const countCField = dawnFieldToC( dawnEntry.length );
			const elem = f.type.element;
			if( elem.kind === "handle" )
				return { f, cField, kind: "seq-handle",
					elemWrapper: elem.name, countCField };
			if( elem.kind === "enum" )
				return { f, cField, kind: "seq-enum",
					elemEnum: elem.name, countCField };
			if( elem.kind === "primitive" && !nonRotePrimitives.has( elem.name ) )
				return { f, cField, kind: "seq-prim",
					cType: elem.cType, countCField };
			if( elem.kind === "dict" && genDicts.has( elem.name ) )
				return { f, cField, kind: "seq-dict",
					dictName: elem.name, countCField };
			return { f, kind: "skip", reason: `seq<${elem.kind}> not yet supported` };
		}
		return { f, kind: "skip", reason: `unsupported field kind ${f.type.kind}` };
	} );

	const stringFields = emits.filter( e => e.kind === "string" ).map( e => e.f );
	const dictFields   = emits.filter( e => e.kind === "dict" );
	const seqFields    = emits.filter( e => e.kind === "seq-handle"
	                                     || e.kind === "seq-enum"
	                                     || e.kind === "seq-prim"
	                                     || e.kind === "seq-dict" );

	function seqElemCType( e ) {
		if( e.kind === "seq-handle" ) return "WGPU" + e.elemWrapper.replace( /^GPU/, "" );
		if( e.kind === "seq-enum" )   return "WGPU" + e.elemEnum.replace( /^GPU/, "" );
		if( e.kind === "seq-prim" )   return e.cType;
		if( e.kind === "seq-dict" )   return "WGPU" + e.dictName.replace( /^GPU/, "" );
		return "void";
	}

	const memberDecls = [
		...stringFields.map( f => `\tWGPU_StringViewHolder ${f.name}_h_;` ),
		...dictFields.map( e => `\twgpu_read_${e.dictName} ${e.f.name}_r_;` ),
		...seqFields.flatMap( e => {
			const cT = seqElemCType( e );
			const m = [
				`\t${cT}* ${e.f.name}_arr_;`,
				`\tsize_t ${e.f.name}_count_;`,
			];
			if( e.kind === "seq-dict" )
				m.push( `\twgpu_read_${e.dictName}** ${e.f.name}_readers_;` );
			return m;
		} ),
	];

	const bodyLines = [];
	bodyLines.push( `\t\tdesc = ${initMacro};` );
	for( const e of emits ) {
		const f = e.f;
		const k = `String::NewFromUtf8Literal( isolate, "${f.name}" )`;
		const hasIt = `opts->Has( context, ${k} ).FromMaybe( false )`;
		const getIt = `opts->Get( context, ${k} ).ToLocalChecked()`;
		switch( e.kind ) {
			case "skip":
				bodyLines.push( `\t\t/* TODO field ${f.name}: ${e.reason} */` );
				break;
			case "bool":
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = ${getIt}->BooleanValue( isolate );` );
				break;
			case "optional-bool":
				// WGPUOptionalBool is tri-state (False=0, True=1, Undefined=2).
				// Leave it Undefined unless JS supplied a value.
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = ${getIt}->BooleanValue( isolate ) ? WGPUOptionalBool_True : WGPUOptionalBool_False;` );
				break;
			case "string":
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = ${f.name}_h_.view();` );
				break;
			case "i32":
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = (${e.c})${getIt}->Int32Value( context ).FromMaybe( 0 );` );
				break;
			case "i64":
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = (${e.c})${getIt}->IntegerValue( context ).FromMaybe( 0 );` );
				break;
			case "float":
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = (${e.c})${getIt}->NumberValue( context ).FromMaybe( 0 );` );
				break;
			case "enum-todo":
				bodyLines.push( `\t\t/* TODO field ${f.name}: enum ${f.type.name} not in dawn pin */` );
				break;
			case "enum": {
				const wgpuT = "WGPU" + e.enumName.replace( /^GPU/, "" );
				// Pre-apply the IDL default if there is one — the dawn INIT
				// macros usually set enum fields to Undefined, which the
				// runtime rejects as invalid for actual bindings. The IDL
				// default ("uniform" for GPUBufferBindingType, "all" for
				// color write masks, etc.) is the spec-correct fallback.
				if( f.default && f.default.type === "string" ) {
					const idlDflt = f.default.value;
					const dawnDflt = ( enumIdlToDawn.get( e.enumName ) || new Map() )
						.get( idlDflt );
					if( dawnDflt !== undefined )
						bodyLines.push( `\t\tdesc.${e.cField} = (${wgpuT})${dawnDflt};  /* IDL default: "${idlDflt}" */` );
				}
				bodyLines.push( `\t\tif( ${hasIt} ) {` );
				bodyLines.push( `\t\t\tv8::String::Utf8Value _s( isolate, ${getIt} );` );
				bodyLines.push( `\t\t\tdesc.${e.cField} = wgpu_str_to_${e.enumName}( *_s, (size_t)_s.length(), desc.${e.cField} );` );
				bodyLines.push( `\t\t}` );
				break;
			}
			case "dict-todo":
				bodyLines.push( `\t\t/* TODO field ${f.name}: nested dict ${f.type.name} not GEN-able */` );
				break;
			case "dict":
				// Nested reader member already constructed via init list;
				// just copy its desc into the parent's struct, or take its
				// address if dawn uses pointer-to-struct.
				bodyLines.push( `\t\tif( ${hasIt} ) desc.${e.cField} = ${e.isPointer ? "&" : ""}${f.name}_r_.desc;` );
				break;
			case "handle": {
				const wrapper = f.type.name;
				bodyLines.push( `\t\tif( ${hasIt} && ${getIt}->IsObject() ) {`);
				bodyLines.push( `\t\t\t${wrapper}* w = node::ObjectWrap::Unwrap<${wrapper}>( ${getIt}.As<v8::Object>() );` );
				bodyLines.push( `\t\t\tdesc.${e.cField} = w->handle_;` );
				bodyLines.push( `\t\t}` );
				break;
			}
			case "seq-handle":
			case "seq-enum":
			case "seq-prim":
			case "seq-dict": {
				const cT = seqElemCType( e );
				bodyLines.push( `\t\tif( ${hasIt} && ${getIt}->IsArray() ) {` );
				bodyLines.push( `\t\t\tv8::Local<v8::Array> _arr = ${getIt}.As<v8::Array>();` );
				bodyLines.push( `\t\t\t${f.name}_count_ = (size_t)_arr->Length();` );
				bodyLines.push( `\t\t\tif( ${f.name}_count_ ) ${f.name}_arr_ = new ${cT}[${f.name}_count_]();` );
				if( e.kind === "seq-dict" )
					bodyLines.push( `\t\t\tif( ${f.name}_count_ ) ${f.name}_readers_ = new wgpu_read_${e.dictName}*[${f.name}_count_]();` );
				bodyLines.push( `\t\t\tfor( size_t _i = 0; _i < ${f.name}_count_; ++_i ) {` );
				bodyLines.push( `\t\t\t\tv8::Local<v8::Value> _v = _arr->Get( context, (uint32_t)_i ).ToLocalChecked();` );
				if( e.kind === "seq-handle" ) {
					bodyLines.push( `\t\t\t\tif( _v->IsObject() ) {` );
					bodyLines.push( `\t\t\t\t\t${e.elemWrapper}* _w = node::ObjectWrap::Unwrap<${e.elemWrapper}>( _v.As<v8::Object>() );` );
					bodyLines.push( `\t\t\t\t\t${f.name}_arr_[_i] = _w->handle_;` );
					bodyLines.push( `\t\t\t\t}` );
				} else if( e.kind === "seq-prim" ) {
					if( /^u?int(8|16|32)_t$/.test( e.cType ) || e.cType === "size_t" )
						bodyLines.push( `\t\t\t\t${f.name}_arr_[_i] = (${e.cType})_v->Int32Value( context ).FromMaybe( 0 );` );
					else if( /^u?int64_t$/.test( e.cType ) )
						bodyLines.push( `\t\t\t\t${f.name}_arr_[_i] = (${e.cType})_v->IntegerValue( context ).FromMaybe( 0 );` );
					else
						bodyLines.push( `\t\t\t\t${f.name}_arr_[_i] = (${e.cType})_v->NumberValue( context ).FromMaybe( 0 );` );
				} else if( e.kind === "seq-enum" ) {
					bodyLines.push( `\t\t\t\tv8::String::Utf8Value _s( isolate, _v );` );
					bodyLines.push( `\t\t\t\t${f.name}_arr_[_i] = wgpu_str_to_${e.elemEnum}( *_s, (size_t)_s.length(), (${cT})0 );` );
				} else /* seq-dict */ {
					bodyLines.push( `\t\t\t\tv8::Local<v8::Object> _o = _v->IsObject() ? _v.As<v8::Object>() : v8::Object::New( isolate );` );
					bodyLines.push( `\t\t\t\t${f.name}_readers_[_i] = new wgpu_read_${e.dictName}( isolate, context, _o );` );
					bodyLines.push( `\t\t\t\t${f.name}_arr_[_i] = ${f.name}_readers_[_i]->desc;` );
				}
				bodyLines.push( `\t\t\t}` );
				bodyLines.push( `\t\t\tdesc.${e.countCField} = ${f.name}_count_;` );
				bodyLines.push( `\t\t\tdesc.${e.cField} = ${f.name}_arr_;` );
				bodyLines.push( `\t\t}` );
				break;
			}
		}
	}

	const initParts = [];
	for( const f of stringFields ) {
		const k = `String::NewFromUtf8Literal( isolate, "${f.name}" )`;
		const has = `opts->Has( context, ${k} ).FromMaybe( false )`;
		const get = `opts->Get( context, ${k} ).ToLocalChecked()`;
		initParts.push( `${f.name}_h_( isolate, ${has} ? ${get} : v8::Local<v8::Value>::Cast( v8::String::Empty( isolate ) ) )` );
	}
	for( const e of dictFields ) {
		const f = e.f;
		const k = `String::NewFromUtf8Literal( isolate, "${f.name}" )`;
		const has = `opts->Has( context, ${k} ).FromMaybe( false )`;
		const get = `opts->Get( context, ${k} ).ToLocalChecked()`;
		// Nested reader receives the inner object, or an empty Object if the
		// field is absent so its constructor still runs safely.
		initParts.push( `${f.name}_r_( isolate, context, ( ${has} && ${get}->IsObject() ) ? ${get}.As<v8::Object>() : v8::Object::New( isolate ) )` );
	}
	for( const e of seqFields ) {
		initParts.push( `${e.f.name}_arr_( NULL )` );
		initParts.push( `${e.f.name}_count_( 0 )` );
		if( e.kind === "seq-dict" )
			initParts.push( `${e.f.name}_readers_( NULL )` );
	}
	const initList = initParts.length ? "\t\t: " + initParts.join( ", " ) : "";

	// Destructor: free heap-allocated sequence storage. For seq-of-dict we
	// also delete each element reader (which in turn frees any nested
	// allocations through this same dtor pattern).
	const dtorLines = [];
	for( const e of seqFields ) {
		if( e.kind === "seq-dict" ) {
			dtorLines.push( `\t\tif( ${e.f.name}_readers_ ) {` );
			dtorLines.push( `\t\t\tfor( size_t _i = 0; _i < ${e.f.name}_count_; ++_i )` );
			dtorLines.push( `\t\t\t\tdelete ${e.f.name}_readers_[_i];` );
			dtorLines.push( `\t\t\tdelete[] ${e.f.name}_readers_;` );
			dtorLines.push( `\t\t}` );
		}
		dtorLines.push( `\t\tif( ${e.f.name}_arr_ ) delete[] ${e.f.name}_arr_;` );
	}

	const out = [
		`struct wgpu_read_${name} {`,
		`\t${wgpuT} desc;`,
		...memberDecls,
		`\twgpu_read_${name}( v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> opts )`,
		initList,
		`\t{`,
		...bodyLines,
		`\t}`,
	];
	if( dtorLines.length ) {
		out.push( `\t~wgpu_read_${name}() {` );
		for( const l of dtorLines ) out.push( l );
		out.push( `\t}` );
	}
	out.push( `};` );
	out.push( `` );
	return out.filter( Boolean ).join( "\n" );
}

// Emit the return statement for a given classified return type, given the
// raw C value already in `cval`.
function emitReturn( ret, cval ) {
	if( ret.kind === "primitive" ) {
		if( ret.cType === "void" ) return `\t(void)(${cval});`;
		if( ret.cType === "bool" )
			return `\targs.GetReturnValue().Set( v8::Boolean::New( isolate, !!(${cval}) ) );`;
		if( ret.cType === "WGPUStringView" )
			return `\targs.GetReturnValue().Set( WGPU_StringViewToV8( isolate, (${cval}) ) );`;
		if( /^u?int(8|16|32)_t$/.test( ret.cType ) )
			return `\targs.GetReturnValue().Set( v8::Integer::NewFromUnsigned( isolate, (uint32_t)(${cval}) ) );`;
		if( /^u?int64_t$/.test( ret.cType ) )
			return `\targs.GetReturnValue().Set( v8::Number::New( isolate, (double)(${cval}) ) );`;
		if( ret.cType === "float" || ret.cType === "double" )
			return `\targs.GetReturnValue().Set( v8::Number::New( isolate, (double)(${cval}) ) );`;
	}
	if( ret.kind === "enum" ) {
		return `\targs.GetReturnValue().Set( v8::Integer::New( isolate, (int32_t)(${cval}) ) );`;
	}
	if( ret.kind === "handle" ) {
		return `\tWGPU_RETURN_NEW( ${ret.name}, (${cval}) );`;
	}
	return `\t/* TODO return ${ret.kind} */`;
}

// Collect the set of wrapper classes a method body depends on (return type +
// any handle args). The owning class is implicit and always present.
function methodDeps( classification ) {
	const deps = new Set();
	for( const a of classification.args ) {
		if( a.type.kind === "handle" ) deps.add( a.type.name );
		else if( a.type.kind === "dict" ) {
			// Pull in the dict reader's transitive deps so the method can
			// instantiate the reader struct at all.
			for( const d of ( dictDepsByName.get( a.type.name ) || [] ) )
				deps.add( d );
		}
	}
	if( classification.ret.kind === "handle" )
		deps.add( classification.ret.name );
	return [ ...deps ];
}

// Emit a getter for a readonly attribute on an object-backed interface.
// Returns null if the attribute can't be expressed (handle return without
// addref handling, complex type, etc.) — caller falls back to HAND.
function emitAttrGetter( cls, attrNode, dawnClassKey, dawnGetterName ) {
	const ret = classifyType( attrNode.idlType );
	const cFn = wgpuFn( dawnClassKey, dawnGetterName );
	const fnName = `gen_get_${cls}_${attrNode.name}`;
	let body;
	if( ret.kind === "primitive" ) {
		const c = ret.cType;
		if( c === "void" ) return null;
		const callC = `${cFn}( self->handle_ )`;
		if( c === "bool" )
			body = `\tinfo.GetReturnValue().Set( v8::Boolean::New( isolate, ${callC} ) );`;
		else if( /^u?int(8|16|32)_t$/.test( c ) || c === "size_t" )
			body = `\tinfo.GetReturnValue().Set( v8::Integer::NewFromUnsigned( isolate, (uint32_t)${callC} ) );`;
		else if( /^u?int64_t$/.test( c ) )
			body = `\tinfo.GetReturnValue().Set( v8::Number::New( isolate, (double)${callC} ) );`;
		else if( c === "float" || c === "double" )
			body = `\tinfo.GetReturnValue().Set( v8::Number::New( isolate, (double)${callC} ) );`;
		else return null;
	} else if( ret.kind === "enum" ) {
		// Enum return → reverse lookup → JS string.
		const wgpuT = "WGPU" + ret.name.replace( /^GPU/, "" );
		body =
`\t${wgpuT} _v = ${cFn}( self->handle_ );
\tconst char* _s = wgpu_str_from_${ret.name}( _v );
\tif( _s ) info.GetReturnValue().Set( v8::String::NewFromUtf8( isolate, _s ).ToLocalChecked() );
\telse info.GetReturnValue().Set( v8::Integer::New( isolate, (int)_v ) );`;
	} else {
		// Handle-return attributes (device.queue, etc.) need addref + cache
		// per spec [SameObject]. Punt to HAND for now.
		return null;
	}
	const v8_major = process.versions.v8.split('.')[0];
console.log( "MAJOR VERSION:", v8_major );
	const thisObj = (Number(process.versions.v8.split('.')[0])>13)?"info.HolderV2()":"info.This()";
	return [
		`// ${cls}.${attrNode.name}  →  ${cFn}`,
		`static void ${fnName}( v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info ) {`,
		`\tv8::Isolate* isolate = info.GetIsolate();`,
		`\t${cls}* self = node::ObjectWrap::Unwrap<${cls}>( ${thisObj} );`,
		`\t(void)property;`,
		body,
		`}`,
	].join( "\n" );
}

function emitMethod( cls, opNode, classification, dawnClassKey, dawnMethodName ) {
	const fnName = `gen_${cls}_${opNode.name}`;
	const cFn = wgpuFn( dawnClassKey, dawnMethodName );
	const reads = classification.args.map( ( a, i ) => emitArgRead( i, a, cls ) );
	const declBlock = reads.map( r => r.decls ).join( "\n" );
	const callArgs = [ "self->handle_", ...reads.map( r => r.value ) ].join( ", " );
	const afterBlock = reads.map( r => r.after ).filter( Boolean ).join( "\n" );
	const ret = classification.ret;
	let callLine;
	if( ret.kind === "primitive" && ret.cType === "void" ) {
		callLine = `\t${cFn}( ${callArgs} );`;
	} else if( ret.kind === "handle" ) {
		const wgpuT = "WGPU" + ret.name.replace( /^GPU/, "" );
		callLine = `\t${wgpuT} _ret = ${cFn}( ${callArgs} );`;
	} else {
		const cT = ret.kind === "enum"
			? `WGPU${ret.name.replace(/^GPU/, "")}`
			: ret.cType;
		callLine = `\t${cT} _ret = ${cFn}( ${callArgs} );`;
	}
	const returnLine = ret.kind === "primitive" && ret.cType === "void"
		? ""
		: emitReturn( ret, "_ret" );
	return [
		`// ${cls}.${opNode.name}  →  ${cFn}`,
		`static void ${fnName}( const v8::FunctionCallbackInfo<v8::Value>& args ) {`,
		`\tWGPU_THIS( ${cls} );`,
		declBlock,
		callLine,
		afterBlock,    // delete[] for any sequence args, runs before return
		returnLine,
		`}`,
		``,
	].filter( Boolean ).join( "\n" );
}

// ---------- enum string→int tables ----------
//
// For each IDL enum that has a matching dawn enum, emit a small inline lookup
// function. JS code passes the kebab-case string ("high-performance"); the
// lookup matches against the dawn names (space-separated lowercase) after
// kebab→space conversion and returns the WGPU<Enum> value, or a default if
// nothing matches.

// Normalize an enum value name for cross-source matching. Strips case,
// whitespace, and hyphens so all of these compare equal:
//   IDL  "rgba8unorm-srgb"  →  "rgba8unormsrgb"
//   dawn "RGBA8 unorm srgb" →  "rgba8unormsrgb"
//   dawn jsrepr "'rgba8unorm-srgb'" → "rgba8unormsrgb"
// IDL empty string represents the unfortunate "undefined" / no-value entry.
function normalizeEnumValue( s ) {
	if( !s ) return "undefined";
	return s.replace( /^'|'$/g, "" )    // strip dawn jsrepr quotes if present
	        .toLowerCase()
	        .replace( /[\s\-]+/g, "" );
}

// Shared map: enumIdlToDawn[idlEnumName][idlValueString] → dawn integer.
// Used by emitDictReader to translate IDL defaults like
// `GPUBufferBindingType type = "uniform"` into the matching Dawn value
// before the conditional read overwrites it.
const enumIdlToDawn = new Map();

function emitEnumLookup( name ) {
	const dawnKey = idlInterfaceToDawn( name );
	const dawnEntry = dawnJson[ dawnKey ];
	if( !dawnEntry || dawnEntry.category !== "enum" ) return null;
	const wgpuT = "WGPU" + name.replace( /^GPU/, "" );
	const idlValues = enums.get( name ).values.map( v => v.value );

	// Build dawn lookup keyed by the normalized name. Prefer jsrepr (the
	// IDL/JS-side string, which dawn.json includes for the unusual cases)
	// when present, since it's authoritative.
	const dawnByNorm = new Map();
	for( const v of dawnEntry.values ) {
		if( v.jsrepr ) dawnByNorm.set( normalizeEnumValue( v.jsrepr ), v.value );
		dawnByNorm.set( normalizeEnumValue( v.name ), v.value );
	}

	// Populate shared map for default-lookup at dict-reader emit time.
	const perEnum = new Map();
	for( const idlVal of idlValues ) {
		const dawnVal = dawnByNorm.get( normalizeEnumValue( idlVal ) );
		if( dawnVal !== undefined ) perEnum.set( idlVal, dawnVal );
	}
	enumIdlToDawn.set( name, perEnum );

	const lines = [];
	// Forward: string → int. JS passes the IDL form ("bgra8unorm");
	// we memcmp it as-is so the binary stays UTF-8 byte-identical.
	lines.push( `static inline ${wgpuT} wgpu_str_to_${name}( const char* s, size_t len, ${wgpuT} dflt ) {` );
	lines.push( `\tif( !s ) return dflt;` );
	for( const idlVal of idlValues ) {
		const dawnVal = dawnByNorm.get( normalizeEnumValue( idlVal ) );
		if( dawnVal === undefined ) continue;  // IDL value newer than dawn pin
		lines.push( `\tif( len == ${idlVal.length} && memcmp( s, "${idlVal}", ${idlVal.length} ) == 0 ) return (${wgpuT})${dawnVal};` );
	}
	lines.push( `\treturn dflt;` );
	lines.push( `}` );

	// Reverse: int → string. Returns the IDL form so JS sees the spec name.
	lines.push( `static inline const char* wgpu_str_from_${name}( ${wgpuT} v ) {` );
	lines.push( `\tswitch( (int)v ) {` );
	const emittedVals = new Set();
	for( const idlVal of idlValues ) {
		const dawnVal = dawnByNorm.get( normalizeEnumValue( idlVal ) );
		if( dawnVal === undefined || emittedVals.has( dawnVal ) ) continue;
		emittedVals.add( dawnVal );
		lines.push( `\t\tcase ${dawnVal}: return "${idlVal}";` );
	}
	lines.push( `\t}` );
	lines.push( `\treturn NULL;` );
	lines.push( `}` );

	return lines.join( "\n" );
}

const enumLookupBlocks = [];
const genEnums = new Set();  // IDL enum names for which a lookup was emitted
for( const name of enums.keys() ) {
	const block = emitEnumLookup( name );
	if( block ) { enumLookupBlocks.push( block ); genEnums.add( name ); }
}

// ---------- bitmask namespace constants ----------
//
// IDL declares bitmasks as `namespace GPUBufferUsage { const ... MAP_READ = 1; }`.
// JS code wants `GPUBufferUsage.MAP_READ` available at global scope. We emit a
// C++ function that builds the JS object at module init.

const namespaceBlocks = [];
for( const [ name, ns ] of namespaces ) {
	const consts = ns.members.filter( m => m.type === "const" );
	if( !consts.length ) continue;
	const lines = [ `\t{` ];
	lines.push( `\t\tv8::Local<v8::Object> ns = v8::Object::New( isolate );` );
	for( const c of consts ) {
		const val = c.value && c.value.value;
		const v = typeof val === "string" && val.startsWith( "0x" )
			? parseInt( val, 16 ) : Number( val );
		lines.push( `\t\tSET_READONLY( ns, "${c.name}", v8::Integer::NewFromUnsigned( isolate, ${v}u ) );` );
	}
	lines.push( `\t\t(void)globalObj->Set( context, v8::String::NewFromUtf8Literal( isolate, "${name}" ), ns );` );
	lines.push( `\t}` );
	namespaceBlocks.push( lines.join( "\n" ) );
}

// Emit dict reader struct definitions. These go into the .cc and are used
// by emitArgRead for dict-typed method args. Each reader gets guarded by any
// wrapper-class dependencies of its handle fields.
// Topological sort so parent readers (with nested-dict members) come after
// their children — required because nested readers are by-value members.
function topoSortDicts( dictNames ) {
	const sorted = [];
	const visited = new Set();
	const visiting = new Set();
	function visit( name ) {
		if( visited.has( name ) || visiting.has( name ) ) return;
		visiting.add( name );
		const dict = dictionaries.get( name );
		if( dict ) {
			for( const f of collectDictFields( dict ) ) {
				if( f.type.kind === "dict" && genDicts.has( f.type.name ) )
					visit( f.type.name );
				else if( f.type.kind === "sequence"
				         && f.type.element.kind === "dict"
				         && genDicts.has( f.type.element.name ) )
					visit( f.type.element.name );
			}
		}
		visiting.delete( name );
		visited.add( name );
		sorted.push( name );
	}
	for( const n of dictNames ) visit( n );
	return sorted;
}

const dictReaderBlocks = [];
const dictDepsByName = new Map(); // name → string[] of WGPU_HAVE_* deps
for( const name of topoSortDicts( [ ...genDicts ] ) ) {
	const dict = dictionaries.get( name );
	if( !dict ) continue;
	const fields = collectDictFields( dict );
	const deps = dictDeps( fields );
	dictDepsByName.set( name, deps );
	const body = emitDictReader( name, fields );
	const guard = methodGuard( deps );
	dictReaderBlocks.push( guard ? `${guard}\n${body}\n#endif\n` : body );
}

// Walk classes, group GEN methods, emit.
const emittedClasses = new Map(); // cls -> { methods: [{name, body}], dawnKey }
for( const [ name, iface ] of interfaces ) {
	if( ( name in interfaceOverrides ) && interfaceOverrides[ name ] === "" )
		continue;
	const dawnKey = idlInterfaceToDawn( name );
	const dawnObj = dawnKey ? dawnObjects[ dawnKey ] : null;
	if( !dawnObj ) continue;
	// IDL allows multiple overloads of the same operation (short + long
	// forms of copyBufferToBuffer, setBindGroup with/without dynamic
	// offsets, etc.). Dawn's C API only has the long form. Group by name
	// and pick the overload whose arg count matches dawn's; fall back to
	// the longest IDL overload if no exact match.
	const byName = new Map();
	for( const m of interfaceMembers.get( name ) || [] ) {
		if( m.type !== "operation" ) continue;
		if( !byName.has( m.name ) ) byName.set( m.name, [] );
		byName.get( m.name ).push( m );
	}
	const methods = [];
	for( const [ opName, candidates ] of byName ) {
		const opDawn = idlOpToDawn( opName );
		if( !dawnObj.methods.has( opDawn ) ) continue;
		const dawnMethod = dawnObj.methods.get( opDawn );
		const dawnArgCount = ( dawnMethod.args || [] ).length;
		// Sequences expand to a (count, ptr) pair on the C side, so an
		// IDL sequence<T> arg counts as TWO C args when comparing to
		// dawn's prototype. e.g. setBindGroup(idx, group, sequence<u32>)
		// has 3 IDL args but 4 C args, matching dawn's 4-arg signature.
		const expandedCount = ( opNode ) => {
			let n = 0;
			for( const a of opNode.arguments || [] ) {
				const t = classifyType( a.idlType );
				n += ( t.kind === "sequence" ) ? 2 : 1;
			}
			return n;
		};
		candidates.sort( ( a, b ) => {
			const ac = expandedCount( a );
			const bc = expandedCount( b );
			const aMatch = ac === dawnArgCount ? 0 : 1;
			const bMatch = bc === dawnArgCount ? 0 : 1;
			if( aMatch !== bMatch ) return aMatch - bMatch;
			return bc - ac;
		} );
		const m = candidates[ 0 ];
		if( handWrittenMethods.has( `${name}.${m.name}` ) ) continue;
		const c = classifyOperation( m, dawnObj, dawnMethod );
		if( c.tag !== "GEN" ) continue;
		const body = emitMethod( name, m, c, dawnKey, opDawn );
		const deps = methodDeps( c );
		methods.push( { name: m.name, body, deps } );
	}

	// Readonly attribute getters. Mirrors method walk: classify each attr's
	// dawn getter, emit a SetNativeDataProperty-shaped callback, register
	// in the wire function. label is universal & wrapper-stored — skipped.
	const attrs = [];
	for( const m of interfaceMembers.get( name ) || [] ) {
		if( m.type !== "attribute" ) continue;
		if( m.name in universalAttributes ) continue;
		const getDawn = idlAttrToDawnGetter( m.name );
		if( !dawnObj.methods.has( getDawn ) ) continue;
		const body = emitAttrGetter( name, m, dawnKey, getDawn );
		if( !body ) continue;  // hand-written (handle-return, complex type)
		attrs.push( { name: m.name, body } );
	}
	if( methods.length || attrs.length )
		emittedClasses.set( name, { methods, attrs, dawnKey } );
}

// Build the .h and .cc.
const hLines = [
	`// Auto-generated by gen.mjs — do not edit.`,
	`// Contains: enum string↔int lookups, dict reader structs, and one`,
	`// wireGenerated_<Class> forward decl per class with GEN-able methods.`,
	`// Hand-written Init() calls these to attach the generated prototype methods.`,
	`// Dict readers are visible here so hand-written code (e.g. requestAdapter)`,
	`// can also use them for option-bag parsing.`,
	``,
	`#pragma once`,
	``,
	`#include "../../../global.h"`,
	`#include "../webgpu_module.h"   // wrapper class decls (GPUBuffer, GPUTextureView, …)`,
	`#include "../webgpu_bindings.h"`,
	`#include "webgpu/webgpu.h"`,
	`#include <cstring>`,
	``,
	`// ---- enum string→int lookups ----`,
	``,
	...enumLookupBlocks,
	``,
	`// ---- dict readers (used by methods with descriptor args) ----`,
	``,
	...dictReaderBlocks,
	``,
];
const ccLines = [
	`// Auto-generated by gen.mjs — do not edit.`,
	``,
	`#include "../webgpu_module.h"`,
	`#include "../webgpu_bindings.h"`,
	`#include "webgpu_generated.h"`,
	``,
];

function methodGuard( deps ) {
	// Owning class guard is provided by the outer #ifdef WGPU_HAVE_<cls>.
	if( !deps.length ) return null;
	const checks = deps.map( d => `defined(WGPU_HAVE_${d})` ).join( " && " );
	return `#if ${checks}`;
}

for( const [ cls, { methods, attrs } ] of emittedClasses ) {
	hLines.push( `#ifdef WGPU_HAVE_${cls}` );
	hLines.push( `extern void wireGenerated_${cls}( v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> tpl );` );
	hLines.push( `#endif` );
	hLines.push( `` );

	ccLines.push( `#ifdef WGPU_HAVE_${cls}` );
	for( const m of methods ) {
		const guard = methodGuard( m.deps );
		if( guard ) ccLines.push( guard );
		ccLines.push( m.body );
		if( guard ) ccLines.push( `#endif  // ${guard.slice(4)}` );
	}
	for( const a of attrs ) ccLines.push( a.body );
	ccLines.push( `void wireGenerated_${cls}( v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> tpl ) {` );
	for( const m of methods ) {
		const guard = methodGuard( m.deps );
		if( guard ) ccLines.push( `\t${guard}` );
		ccLines.push( `\tNODE_SET_PROTOTYPE_METHOD( tpl, "${m.name}", gen_${cls}_${m.name} );` );
		if( guard ) ccLines.push( `\t#endif` );
	}
	// Register attribute getters via SetNativeDataProperty on the prototype
	// template — matches com_interface.cc's pattern.
	if( attrs.length ) {
		ccLines.push( `\tv8::Local<v8::Context> context = isolate->GetCurrentContext();` );
		ccLines.push( `\t(void)context;` );
		for( const a of attrs ) {
			ccLines.push(
`\ttpl->PrototypeTemplate()->SetNativeDataProperty(
\t\tv8::String::NewFromUtf8Literal( isolate, "${a.name}" ),
\t\tgen_get_${cls}_${a.name} );` );
		}
	}
	ccLines.push( `}` );
	ccLines.push( `#endif  // WGPU_HAVE_${cls}` );
	ccLines.push( `` );
}

// ---- bitmask namespace constants ----
hLines.push( `extern void wireGeneratedConstants( v8::Isolate* isolate, v8::Local<v8::Object> globalObj );` );
hLines.push( `` );

ccLines.push( `// ---- bitmask namespace constants ----` );
ccLines.push( `` );
ccLines.push( `void wireGeneratedConstants( v8::Isolate* isolate, v8::Local<v8::Object> globalObj ) {` );
ccLines.push( `\tv8::Local<v8::Context> context = isolate->GetCurrentContext();` );
for( const blk of namespaceBlocks ) ccLines.push( blk );
ccLines.push( `}` );

const hPath = join( OUT_DIR, "webgpu_generated.h" );
const ccPath = join( OUT_DIR, "webgpu_generated.cc" );
writeFileSync( hPath, hLines.join( "\n" ) );
writeFileSync( ccPath, ccLines.join( "\n" ) );

// ---------- write manifest + report ----------

const outPath = join( OUT_DIR, "webgpu_generated.manifest" );
writeFileSync( outPath, manifestLines.join( "\n" ) );

function pct( a, b ) { return b ? ( ( 100 * a / b ) | 0 ) + "%" : "—"; }

console.log( "=== WebGPU IDL coverage vs dawn.json ===" );
console.log( `interfaces    ${counts.ifaceMatched}/${counts.interfaces}    ${pct( counts.ifaceMatched, counts.interfaces )}` );
console.log( `operations    ${counts.opMatched}/${counts.operations}    ${pct( counts.opMatched, counts.operations )}` );
console.log( `attributes    ${counts.attrMatched}/${counts.attributes}    ${pct( counts.attrMatched, counts.attributes )}` );
console.log( `dictionaries  ${counts.dictMatched}/${counts.dictionaries}    ${pct( counts.dictMatched, counts.dictionaries )}` );
console.log( `enums         ${counts.enumMatched}/${counts.enums}    ${pct( counts.enumMatched, counts.enums )}` );
console.log( "" );
console.log( "--- emit classification (matched only) ---" );
console.log( `operations    GEN ${counts.opGen}    HAND ${counts.opHand}    SKIP ${counts.opSkip}` );
console.log( `dictionaries  GEN ${counts.dictGen}   HAND ${counts.dictHand}` );
console.log( "" );

function dumpUnmatched( label, list, max = 20 ) {
	if( !list.length ) return;
	console.log( `--- unmatched ${label} (${list.length}) ---` );
	for( const s of list.slice( 0, max ) ) console.log( "  " + s );
	if( list.length > max ) console.log( `  …and ${list.length - max} more` );
	console.log( "" );
}
dumpUnmatched( "interfaces",   unmatched.interfaces );
dumpUnmatched( "operations",   unmatched.operations,   40 );
dumpUnmatched( "attributes",   unmatched.attributes,   40 );
dumpUnmatched( "dictionaries", unmatched.dictionaries, 40 );
dumpUnmatched( "enums",        unmatched.enums );

if( handReasons.length ) {
	console.log( `--- HAND reasons (${handReasons.length}) ---` );
	// Cluster by reason prefix to see what categories dominate.
	const byCategory = new Map();
	for( const line of handReasons ) {
		const reason = line.slice( line.indexOf( ":" ) + 1 ).trim();
		// Normalize "arg \"foo\" is X" → "arg is X"
		const key = reason
			.replace( /arg "[^"]+" is /, "arg is " )
			.replace( /field "[^"]+" is /, "field is " );
		byCategory.set( key, ( byCategory.get( key ) || 0 ) + 1 );
	}
	const sorted = [ ...byCategory.entries() ].sort( ( a, b ) => b[ 1 ] - a[ 1 ] );
	for( const [ cat, n ] of sorted )
		console.log( `  ${String( n ).padStart( 3 )}  ${cat}` );
	console.log( "" );
}

console.log( `manifest written: ${outPath}` );
console.log( `generated:      ${hPath}` );
console.log( `generated:      ${ccPath}` );
let totalGenMethods = 0;
for( const [ , v ] of emittedClasses ) totalGenMethods += v.methods.length;
console.log( `${emittedClasses.size} classes, ${totalGenMethods} methods emitted (under #ifdef WGPU_HAVE_<Class>)` );
