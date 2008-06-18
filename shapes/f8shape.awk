# --------------------------------- f8shape.awk -------------------------------

#
# This is part of the flight simulator 'fly8'.
# Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
#

#
# Convert a (cpp processed) object description to a simple list of numbers.
# The input should be a comma separated list of quadruplets.
# Evaluates expressions using '+ - * / % ( )'. All other characters are 
# removed. A vertex count is added to the beginning.
#


BEGIN	{
	name = ARGV[1]
	fin  = name ".i"
	fout = name ".003"

	lineno = 0
	n = 0
	advance()
	while (1) {
		x = int(expression())
		eat(",")

		y = int(expression())
		eat(",")

		z = int(expression())
		eat(",")

		f = int(expression())
		if (0 == f)
			break;

		printf ("%d %d %d %d\n", x, y, z, f) >fout
		++n

		if (tok == "(eof)")
			break
		eat(",")
	}

	close(fin)
	close(fout)

	fin  = name ".003"
	fout = name ".vxx"

	print n, 2 >fout		# V_METERS = 2
	while (getline<fin != 0) {
		print $0 >fout
	}

	close(fin)
	close(fout)
}

function advance() {
	if (tok == "(eof)")
		return tok
	while (1) {
		while (1) {
			if (length(line) == 0 || line ~ /^static VERTEX/)
				++lineno
			else if (substr(line,1,1) == "#") {
				split(line,a)
				if (a[1] == "#line")
					lineno = a[2]+0
				else
					++lineno
			} else
				break
			if (getline line <fin == 0)
				return tok = "(eof)"
		}
		if (match(line, /^[ \t;\n{}]/))
		    	line = substr(line, 2)
		else
		    	break
	}
	if (match(line, /^[0-9]+/) ||
	    match(line, /^(<=|==|!=|>=|\|\||&&|>>|<<)/) ||
	    match(line, /^./)) {
	    	tok = substr(line, 1, RLENGTH)
	    	line = substr(line, RLENGTH+1)
	    	return tok
	}
	error("line " lineno " incomprehensible at " line)
}

function eat(s) {
	if (tok != s)
		error("line " lineno ": saw \"" tok "\" expected \"" s "\"")
	advance()
}

function error(s) {
	print "Error: " s
	exit 1
}

function expression(  e, e1, e2) {
	e = logical_or()
	if (tok == "?") {
		advance()
		e1 = expression()
		eat(":")
		e2 = expression()
		if (e)
			e = e1
		else
			e = e2
# This fails!:	e = e ? e1 : e2
	}
	return e
}

function logical_or(  e, e1) {
	e = logical_and()
	while (tok == "||") {
		advance()
		e1 = logical_and()
		e = e || e1
	}
	return e
}

function logical_and(  e, e1) {
	e = inclusive_or()
	while (tok == "&&") {
		advance()
		e1 = inclusive_or()
		e = e && e1
	}
	return e
}

function bitwise(a, b, op,  abit, bbit, asign, bsign, rbit, r, t, n) {
	a = int(a)
	asign = (a < 0) ? 1 : 0;
	b = int(b)
	bsign = (b < 0) ? 1 : 0;
	r = 0
	rpos = 1
	for (n = 0; n < 32; ++n) {
		t = int((a-asign)/2)
		abit = a != t+t
		a = t
		t = int((b-bsign)/2)
		bbit = b != t+t
		b = t
		if (op == "|")
			rbit = abit || bbit
		else if (op == "&")
			rbit = abit && bbit
		else if (op == "^")
			rbit = abit != bbit
		else # unary ~
			rbit = !bbit
		r += rpos*rbit
		rpos += rpos
	}
	return r
}

function inclusive_or(  e, e1) {
	e = exclusive_or()
	while (tok == "|") {
		advance()
		e1 = exclusive_or()
		e = bitwise(e, e1, "|");
	}
	return e
}

function exclusive_or(  e, e1) {
	e = and()
	while (tok == "^") {
		advance()
		e1 = and()
		e = bitwise(e, e1, "^");
	}
	return e
}

function and(  e, e1) {
	e = equality()
	while (tok == "&") {
		advance()
		e1 = equality()
		e = bitwise(e, e1, "&");
	}
	return e
}

function equality(  op, e, e1) {
	e = relational()
	while (tok == "==" || tok == "!=") {
		op = tok
		advance()
		e1 = relational()
		if (op == "==")
			e = e == e1
		else
			e = e != e1
	}
	return e
}

function relational(  op, e, e1) {
	e = shift()
	while (tok ~ /<|<=|>=|>/) {
		op = tok
		advance()
		e1 = shift()
		if (op == "<")
			e = e < e1
		else if (op == "<=")
			e = e <= e1
		else if (op == ">=")
			e = e >= e1
		else
			e = e > e1
	}
	return e
}

function shift(  op, e, e1) {
	e = additive()
	while (tok == "<<" || tok == ">>") {
		op = tok
		advance()
		e1 = additive()
		if (op == ">>")
			e1 = -e1
		if (e1 > 0) {
			for (e1 = int(e1); e1 > 0; --e1)
				e *= 2;
		} else {
			for (e1 = int(-e1); e1 > 0; --e1)
				e /= 2;
		}
	}
	return e
}

function additive(  op, e, e1) {
	e = multiplicative()
	while (tok == "+" || tok == "-") {
		op = tok
		advance()
		e1 = multiplicative()
		if (op == "+")
			e = e + e1
		else
			e = e - e1
	}
	return e
}

function multiplicative(  op, e, e1) {
	e = unary()
	while (tok == "*" || tok == "/" || tok == "%") {
		op = tok
		advance()
		e1 = unary()
		if (op == "*")
			e = e * e1
		else if (op == "/")
			e = e / e1
		else
			e = e % e1
	}
	return e
}

function unary() {
	if (tok == "-") {
		advance()
		return -primary()
	} else if (tok == "~") {
		advance()
		return bitwise(0, primary(), "~")
	} else if (tok == "!") {
		advance()
		return !primary()
	} else if (tok == "+") {
		advance()
		return primary()
	} else
		return primary()
}

function primary(  e) {
	if (tok == "(") {
		advance()
		e = expression()
		eat(")")
		return e
	}

	if (tok ~ /^[0-9]+/) {
		e = tok
		advance()
		return e+0
	}

	error("unexpected \"" tok "\" at line " lineno)
}
