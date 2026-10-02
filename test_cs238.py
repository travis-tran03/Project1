#!/usr/bin/env python3
"""Check cs238 against the expression language and the sigmoid extra credit."""

import math
import os
import random
import re
import subprocess
import sys

NUM = re.compile(r"(?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?")

KNOWN = [
    ("0", 0.0),
    ("1", 1.0),
    ("42", 42.0),
    ("1+2", 3.0),
    ("1+2*3", 7.0),
    ("2*3+4", 10.0),
    ("(1+2)*3", 9.0),
    ("1+2*(3-4)/5", 0.6),
    ("-3+5", 2.0),
    ("+5", 5.0),
    ("+-3", -3.0),
    ("-+3", -3.0),
    ("--5", 5.0),
    ("---5", -5.0),
    ("1/0", 0.0),
    ("0/0", 0.0),
    ("0/1", 0.0),
    ("5/(1-1)", 0.0),
    ("1/0/2", 0.0),
    ("8/2/2", 2.0),
    ("8-3-2", 3.0),
    ("2*3*4", 24.0),
    ("10/4", 2.5),
    ("1/2", 0.5),
    ("1/2/2", 0.25),
    ("4/2*3", 6.0),
    ("1+2+3+4", 10.0),
    ("(2+3)*(4-1)", 15.0),
    ("2*-3", -6.0),
    ("-2*-3", 6.0),
    ("-(2+3)", -5.0),
    ("((2))", 2.0),
    ("  1 + 2 ", 3.0),
    ("1.5+2.5", 4.0),
    ("0.5*4", 2.0),
    (".5*4", 2.0),
    ("5.", 5.0),
    ("(1+2)/(3+4)", 3.0 / 7.0),
    ("10-(3+2)*2", 0.0),
    ("10-3+2", 9.0),
    ("2+3*4-5", 9.0),
    ("2*3+4*5", 26.0),
    ("((1+2)*3)+4", 13.0),
    ("1+(2+(3+4))", 10.0),
    ("-1+2*-3", -7.0),
    ("4*2/3", 8.0 / 3.0),
    ("1++2", 3.0),
    ("1--2", 3.0),
    ("1+-2", -1.0),
    ("1-+2", -1.0),
    ("2/+2", 1.0),
    ("2*-+3", -6.0),
    ("(((1+2)*3)-4)/5", 1.0),
    ("(8-3)*(2+1)/5", 3.0),
    ("100-1*2*3*4", 76.0),
    ("1.25*4", 5.0),
    ("-0+5", 5.0),
    ("3/(1+1)", 1.5),
    ("((4))", 4.0),
    ("1*2*3*4*5", 120.0),
    ("9-8-7-6", -12.0),
    ("1/1/1/2", 0.5),
    ("(1-1)/(2-2)", 0.0),
    ("7*(0)", 0.0),
    ("+-+-2", 2.0),
    ("2*(3+(4*5))", 46.0),
    ("((2+3)*4)-5", 15.0),
    ("10/2-3", 2.0),
    ("10/(2-3)", -10.0),
    ("0.1+0.2", 0.3),
    ("1e2", 100.0),
    ("1e2+3", 103.0),
    ("2.5*2", 5.0),
    ("(1+2)*(3+4)/(5+2)", 3.0),
    ("-(-(-4))", -4.0),
    ("6/3/2*5", 5.0),
    ("1+2-3+4-5+6", 5.0),
    ("(1-1)+4*0", 0.0),
]

INVALID = [
    "",
    " ",
    "+",
    "-",
    "*",
    "/",
    "(",
    ")",
    "()",
    "( )",
    "(1+2",
    "1+2)",
    "1+",
    "2*",
    "3/",
    "1 2",
    "1*/2",
    "1+*2",
    "hello",
    "((1)",
    "2 3 +",
    "(1+)2",
    "1..2",
    "++",
    "--",
    "5+",
    "(2+3)*(4",
]


def tokenize(s):
    toks = []
    i = 0
    while i < len(s):
        c = s[i]
        if c in "+-*/()":
            toks.append(c)
            i += 1
        elif c.isspace():
            i += 1
        else:
            match = NUM.match(s, i)
            if not match:
                raise ValueError("lex")
            toks.append(float(match.group(0)))
            i = match.end()
    return toks


class Parser(object):
    def __init__(self, toks):
        self.toks = toks
        self.i = 0

    def peek(self):
        if self.i < len(self.toks):
            return self.toks[self.i]
        return None

    def primary(self):
        token = self.peek()
        if isinstance(token, float):
            self.i += 1
            return token
        if token == "(":
            self.i += 1
            value = self.additive()
            if self.peek() != ")":
                raise ValueError("paren")
            self.i += 1
            return value
        raise ValueError("primary")

    def unary(self):
        token = self.peek()
        if token == "+":
            self.i += 1
            return self.unary()
        if token == "-":
            self.i += 1
            return -self.unary()
        return self.primary()

    def multiplicative(self):
        value = self.unary()
        while self.peek() in ("*", "/"):
            op = self.peek()
            self.i += 1
            right = self.unary()
            if op == "*":
                value *= right
            else:
                value = 0.0 if right == 0.0 else value / right
        return value

    def additive(self):
        value = self.multiplicative()
        while self.peek() in ("+", "-"):
            op = self.peek()
            self.i += 1
            right = self.multiplicative()
            value = value + right if op == "+" else value - right
        return value

    def parse(self):
        value = self.additive()
        if self.peek() is not None:
            raise ValueError("trailing")
        return value


def evaluate(expr):
    return Parser(tokenize(expr)).parse()


def sigmoid(x):
    if x < 0.0:
        e = math.exp(x)
        return e / (1.0 + e)
    return 1.0 / (1.0 + math.exp(-x))


def gen_unary(depth):
    roll = random.random()
    if roll < 0.12:
        return "+" + gen_unary(depth)
    if roll < 0.24:
        return "-" + gen_unary(depth)
    return gen_primary(depth)


def gen_primary(depth):
    if depth < 5 and random.random() < 0.28:
        return "(" + gen_expr(depth + 1) + ")"
    return str(random.randint(0, 9))


def gen_mult(depth):
    expr = gen_unary(depth)
    while random.random() < 0.35:
        expr += random.choice(["*", "/"]) + gen_unary(depth)
    return expr


def gen_expr(depth=0):
    expr = gen_mult(depth)
    while random.random() < 0.35:
        expr += random.choice(["+", "-"]) + gen_mult(depth)
    return expr


def run(binary, expr):
    proc = subprocess.run(
        [binary, expr],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def main():
    root = os.path.dirname(os.path.abspath(__file__))
    binary = os.path.join(root, "cs238")
    if not os.path.isfile(binary):
        print("missing %s; run make first" % binary, file=sys.stderr)
        return 1

    for expr, expected in KNOWN:
        got = evaluate(expr)
        if abs(got - expected) > 1e-9:
            print("oracle mismatch for %r: %s != %s" % (expr, got, expected))
            return 1

    valid = [expr for expr, _expected in KNOWN]
    random.seed(238)
    seen = set(valid)
    for _ in range(80):
        expr = gen_expr()
        if expr not in seen:
            seen.add(expr)
            valid.append(expr)

    fails = []
    for expr in valid:
        code, out, err = run(binary, expr)
        want = "%.6f" % sigmoid(evaluate(expr))
        if code != 0 or out != want:
            fails.append("valid %r got %r exit %s stderr %r want %s" % (expr, out, code, err, want))

    for expr in INVALID:
        code, out, err = run(binary, expr)
        if code == 0 or out:
            fails.append("invalid %r got %r exit %s stderr %r" % (expr, out, code, err))

    print("valid %d  invalid %d  failures %d" % (len(valid), len(INVALID), len(fails)))
    for row in fails[:40]:
        print("FAIL", row)
    if fails:
        print("FAILED")
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
