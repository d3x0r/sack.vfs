# Small Seedable Pseudo Random Number Generators
This is a small library with 4 small seedable random number generators.

Implemented from [this Stack Overflow](https://stackoverflow.com/questions/521295/seeding-the-random-number-generator-in-javascript) article.

SFC - SImple Fast Counter RNG. sfc32 is part of the PractRand random number testing suite (which it passes of course). sfc32 has a 128-bit state and is very fast in JS.

MUL - Mulberry32 is a simple generator with a 32-bit state, but is extremely fast and has good quality randomness (author states it passes all tests of gjrand testing suite and has a full 232 period, but I haven't verified).

XOR - As of May 2018, xoshiro128** is the new member of the Xorshift family, by Vigna & Blackman (professor Vigna was also responsible for the Xorshift128+ algorithm powering most Math.random implementations under the hood). It is the fastest generator that offers a 128-bit state.

JSF - This is JSF or 'smallprng' by Bob Jenkins (2007), who also made ISAAC and SpookyHash. It passes PractRand tests and should be quite fast, although not as fast as sfc32.

Simple interface takes a string seed value and parses it into a seed.

``` js
// only one of these are required.
import {SFC32} from "sack.vfs/prng"
import {MUL32} from "sack.vfs/prng"
import {XOR32} from "sack.vfs/prng"
import {JSF32} from "sack.vfs/prng"

const RNG1 = SFC32( "seed1" );
const RNG2 = MUL32( "seed2" );
const RNG3 = XOR32( "seed3" );
const RNG4 = JSF32( "seed4" );


const r1 = RNG1(); // get a number from 0 to <1
const r2 = RNG2(); // get a number from 0 to <1
const r3 = RNG3(); // get a number from 0 to <1
const r4 = RNG4(); // get a number from 0 to <1

// the RNG functions can be called repeatedly each time 
// resulting with a new random number betwee 0 and 1 (not including 1).

```


## Custom Seeding

You can build meta seed arrays; the RNG's take an array of 4 32 bit integers as a seed value.

- `getSeed(s)`- returns a PRNG itself; more of a hash function on the string; each call returns a different integer also.
- `getSeed4(s)` - returns an array of 4 integers to be used.
- `SFC32_(array)` - use a seed array to get a SFC RNG.
- `MUL32_(array)` - use a seed array to get a MUL RNG.
- `XOR32_(array)` - use a seed array to get a XOR RNG.
- `JSF32_(array)` - use a seed array to get a JSF RNG.

``` js
import {getSeed} from "sack.vfs/prng";
import {getSeed4} from "sack.vfs/prng";
const seedFunc = getSeed( "Seed String");
import {SFC32_} from "sack.prng";
const seedArray = getSeed4( "Seed String" );

SFC32_( [seedFunc(), seedFunc(), seedFunc(), seedFunc()] );

```