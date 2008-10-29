#!/usr/bin/env python
"""\
UTP - Intro. a IA 2008
Bogdano Arendartchuk <debogdano@gmail.com>

Formato do arquivo de casos:

  <peso0>   <peso1>   ... <pesoN>
  <classe0> <classe1> ... <classeN> <classe-do-registro>
   .
   .

Formato do arquivo de novo caso:

  <valor0>, <valor1>, ..., <valorN> <- atribs do novo caso
  N, M, O, ...                      <- indice dos atribs continuos

Uso:
    rbc <casos.csv> [<novocaso.csv>]

O arquivo de novo caso e' opcional.
"""
import sys
import csv

class Error(Exception):
    pass

class FormatError(Error):
    pass

class InputError(Error):
    pass

def read_weights(reader):
    cols = reader.next()
    try:
        weights = [float(col) for col in cols if col]
    except ValueError, e:
        raise FormatError, \
                "formato invalido: %s" % e
    return weights

def rbc(newcase, reader, contattribs):
    weights = read_weights(reader)
    wsum = sum(weights)
    ncols = len(weights)
    ranks = []
    lines = []
    minmax = {}
    for line, cols in enumerate(reader):
        lines.append(cols)
        for i in contattribs:
            minmax.setdefault(i, [0,0])
            try:
                value = float(cols[i])
            except ValueError:
                raise FormatError, "a coluna  %d na linha %d deveria "\
                        "ser um numero, mas nao parece ser" % \
                        (i + 1, line + 2)
            cols[i] = value
            if value < minmax[i][0]:
                minmax[i][0] = cols[i]
            elif value > minmax[i][1]:
                minmax[i][1] = cols[i]
    for i, (min, max) in minmax.iteritems():
        minmax[i] = abs(max - min)
    for line, cols in enumerate(lines):
        rank = 0.0
        cols, _class = cols[:-1], cols[-1]
        if len(cols) != ncols:
            raise FormatError, "numero de colunas inesperado na linha " \
                    "%d: esperava %d, encontrou %d" % \
                    (line + 2, ncols, len(cols))
        for i, col in enumerate(cols):
            arank = 0.0
            if i in contattribs:
                # continous attribute, compare using the distance with the
                # new attribute
                arank += abs(newcase[i] - float(cols[i])) / minmax[i]
            else:
                # categorical attribute, compare using equality (0 or 1)
                if cols[i] != newcase[i]:
                    arank += 1.0
                # else: arank += 0
            rank += arank * weights[i]
        rank = rank / wsum
        ranks.append((rank, line + 2, _class))
    ranks.sort()
    return ranks[0]

def ask_questions(path=None):
    if path is None:
        print "Entre o novo caso, com atributos separados por virgula (,):"
        caseline = raw_input("> ")
        print "Indique quais atributos sao continuos. Digite a \n"\
                "a posicao (indice+1) separado por virgula: (eg: 1,5,6)"
        contline = raw_input("> ")
    else:
        f = open(path)
        caseline = f.readline()
        contline = f.readline()
        f.close()
    fields = contline.split(",")
    contattribs = [(int(field.strip()) - 1) for field in fields
            if field.strip()]
    fields = caseline.split(",")
    newcase = []
    for i, field in enumerate(fields):
        field = field.strip()
        if i in contattribs:
            newcase.append(float(field))
        else:
            newcase.append(field)
    return newcase, contattribs

def main(args):
    if len(args) < 2:
        print __doc__
        print
        return 1
    try:
        stream = open(args[1])
        reader = csv.reader(stream, dialect="excel")
        path = len(args) > 2 and args[2] or None
        newcase, contattribs = ask_questions(path)
        rank, line, _class = rbc(newcase, reader, contattribs)
        print "classe %s, na linha %d, f. similaridade: %f (quanto menor "\
                "maior a similaridade)" % (_class, line, rank)
    except (IOError, Error), e:
        sys.stderr.write("error: %s\n" % e)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
