
module.exports = exports = class Fraction {
	numerator = 0;
	denominator = 1n;
	constructor(a,b){
		//console.log( "type:", typeof a, typeof b );
		if( a instanceof Fraction ) {
			this.numerator = a.numerator;
			this.denominator = a.denominator;
		} else if( "number" === typeof a ) {
			if( b === undefined ) {
				this.numerator = BigInt(a);
			} else if( "number" === typeof b ) {
				this.numerator = BigInt(a); this.denominator = BigInt(b);
			} else if( "bigint" === typeof b ) {
				this.numerator = BigInt(a); this.denominator = b;
			}

		} else if( "bigint" === typeof a ) {
			if( b === undefined ) {
				this.numerator = BigInt(a);
			} else if( "number" === typeof b ) {
				this.numerator = BigInt(a); this.denominator = BigInt(b);
			} else if( "bigint" === typeof b ) {
				this.numerator = BigInt(a); this.denominator = b;
			}
		} else {
			// may convert string to a number? Doesn't make a lot of sense for a lot of types.
			this.numerator = BigInt(a);
		}
	}
	add(f) {
		if( f instanceof Fraction ) {
			return new Fraction( this.numerator*f.denominator + f.numerator*this.denominator, this.denominator*f.denominator ).reduce();
		}
	}
	sub(f) {
		if( f instanceof Fraction ) {
			return new Fraction( this.numerator*f.denominator - f.numerator*this.denominator, this.denominator*f.denominator ).reduce();
		}
	}
	mul(f) {
		if( f instanceof Fraction ) {
			return new Fraction( this.numerator * f.numerator, this.denominator*f.denominator ).reduce();
		}
	}
	div(f) {
		if( f instanceof Fraction ) {
			return new Fraction( this.numerator*f.denominator, this.denominator*f.numerator ).reduce();
		}
	}
	scale(n) {
		return n * this.numerator / this.denominator;
	}
	reduce() {
		// GCD from https://stackoverflow.com/questions/17445231/js-how-to-find-the-greatest-common-divisor
		let a = this.numerator<0?-this.numerator:this.numerator;
		let b = this.denominator<0?-this.denominator:this.denominator;
		//if (b > a) {const temp = a; a = b; b = temp;}
		let gcd = 1n;
		while (true) {
			if (!b) { gcd=a; break; }
			a %= b;
			if( !a ) { gcd=b; break; }
			b %= a;
		}
		//console.log( "gcd:", gcd );
		this.numerator = this.numerator/gcd;
		this.denominator = this.denominator/gcd;
		return this;
	}
	toString() {
		if( this.numerator > this.denominator ) {
			const r = this.numerator%this.denominator;
			if( r )
				return `${this.numerator/this.denominator} ${r}/${this.denominator}`
			else
				return `${this.numerator/this.denominator}`
		} else
			return `${this.numerator}/${this.denominator}`;
	}
}
