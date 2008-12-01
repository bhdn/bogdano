#!/usr/bin/env python
# -*- encoding: utf-8
"""
Um interpretador dos mnemônicos da máquina MEPA.

Baseado na descrição de:
http://gerds.utp.br/diogenes/co2/20082-CO2-Aulas2.pdf

"""
import sys
import optparse

__author__ = "Bogdano Arendartchuk <debogdano@gmail.com>"

MEMORY_SIZE = 128
DATA_SEGMENT = 0
STACK_SEGMENT = 0
D_SEGMENT = MEMORY_SIZE - 10
MAX_STACK_SEGMENT = D_SEGMENT - 1

class Error(Exception):
    pass

class InvalidInstruction(Error):
    pass

class AssertFailed(Error):
    pass

class EmptyStack(Error):
    pass

class StackOverflow(Error):
    pass

class ProgramFinished(Error):
    pass

def extension(f):
    "Indica que a função anotada não faz parte da MEPA 'oficial'"
    f.extension = True
    return f

class Memory:

    def __init__(self, size, registers, tag=False):
        self.data = [999999] * size
        self.regs = registers
        self.showtags = tag
        self.tags = {}

    def pop(self):
        if self.regs.sp < STACK_SEGMENT:
            raise EmptyStack
        value = self.data[self.regs.sp]
        try:
            del self.tags[self.regs.sp]
        except KeyError:
            pass
        self.regs.sp -= 1
        return value

    def push(self, value):
        if self.regs.sp >= MAX_STACK_SEGMENT:
            raise StackOverflow
        self.regs.sp += 1
        self.data[self.regs.sp] = value

    def set(self, address, value):
        self.data[address] = value

    def get(self, address):
        return self.data[address]

    def tag(self, address, tag):
        self.tags[address] = tag

    def dump(self, output):
        i = 0
        size = len(self.data)
        columns = 8
        matchseg = False
        for col in xrange(columns):
            output.write("%8d " % col)
        while i < size:
            if i == DATA_SEGMENT:
                output.write("\ndata:")
                matchseg = True
            if i == D_SEGMENT:
                output.write("\n\"D segment\":")
                matchseg = True
            if i == STACK_SEGMENT:
                output.write("\nstack:")
                matchseg = True
            if i % columns == 0 or matchseg:
                output.write("\n%3d: " % i)
                matchseg = False
            if self.showtags:
                tag = self.tags.get(i, "")
            else:
                tag = ""
            output.write("%8d%s " % (self.data[i], tag))
            i += 1
        output.write("\n")

class RegisterSet:

    def __init__(self):
        self.ds = DATA_SEGMENT
        self.ss = STACK_SEGMENT
        self.pc = 0
        self.sp = self.ss - 1
        self.bp = 0

    def dump(self, output):
        output.write("PC: 0x%x, SP: 0x%x\n" % (self.pc, self.sp))

class InstructionSet:
    
    def __init__(self, registers, memory):
        self.regs = registers
        self.mem = memory

    def get(self, name):
        try:
            meth = getattr(self, "i_" + name)
        except AttributeError:
            raise InvalidInstruction, "unknown instruction '%s'" % name
        return meth

    def i_assert(self):
        """Teste da VM

        Compara dois valores da pilha e encerra a execução com erro caso
        os valores sejam diferentes
        """
        value1, value2 = self.mem.pop(), self.mem.pop()
        if value1 != value2:
            raise AssertFailed, "%d != %d" % (value1, value2)

    def i_inspect(self):
        """Chama o depurador de Python"""
        import pdb; pdb.set_trace()

    def i_crct(self):
        "Carrega uma constante na pilha"
        # Estra esta instrução é no-op, pois o valor já está no código e o
        # que está no código é posto na pilha automaticamente.
        # (poderia ter vindo de uma seção de constantes)
        pass

    def i_crvl(self):
        "Carrega um valor da memória na pilha"
        reladdr = self.mem.pop()
        k = self.mem.pop()
        absaddr = self.mem.get(D_SEGMENT + k) + reladdr
        value = self.mem.get(absaddr)
        self.mem.push(value)

    def i_soma(self):
        "Soma dois valores"
        value2, value1 = self.mem.pop(), self.mem.pop()
        sum = value1 + value2
        self.mem.push(sum)

    def i_subt(self):
        "Subtrai dois valores"
        value2, value1 = self.mem.pop(), self.mem.pop()
        sub = value1 - value2
        self.mem.push(sub)

    def i_mult(self):
        "Multiplica dois valores"
        value2, value1 = self.mem.pop(), self.mem.pop()
        mul = value1 * value2
        self.mem.push(mul)

    def i_divi(self):
        "Divide (de inteiros) dois valores"
        value2, value1 = self.mem.pop(), self.mem.pop()
        div = value1 // value2
        self.mem.push(div)

    @extension
    def i_modu(self): # extensao
        "Resto da divisao de dois valores"
        value2, value1 = self.mem.pop(), self.mem.pop()
        mod = value1 % value2
        self.mem.push(mod)

    def i_invr(self):
        "Inverte sinal (WTF?!) de um valor"
        value = self.mem.pop()
        invr = -value
        self.mem.push(invr)

    def i_conj(self):
        "Conjunção entre dois valores (????)"
        #FIXME não está claro que tipo de conjunção é essa. Se é igual a C
        # ou a Pascal, ou se é bit a bit. Vou usar a de Python mesmo.
        value1, value2 = self.mem.pop(), self.mem.pop()
        conj = value1 and value2
        self.mem.push(conj)

    def i_disj(self):
        "Disjunção entre dois valoresi (??????)"
        value1, value2 = self.mem.pop(), self.mem.pop()
        disj = value1 or value2
        self.mem.push(disj)

    def i_nega(self):
        "Negação de um valor"
        value = self.mem.pop()
        invr = int(not value)
        self.mem.push(invr)

    def i_cmme(self):
        "Compara se menor"
        value2, value1 = self.mem.pop(), self.mem.pop()
        cmme = int(value1 < value2)
        self.mem.push(cmme)

    def i_cmma(self):
        "Compara se maior"
        value2, value1 = self.mem.pop(), self.mem.pop()
        cmme = int(value1 > value2)
        self.mem.push(cmme)

    def i_cmig(self):
        "Compara se igual"
        value2, value1 = self.mem.pop(), self.mem.pop()
        cmig = int(value1 == value2)
        self.mem.push(cmig)

    def i_cmdg(self):
        "Compara se desigual"
        value2, value1 = self.mem.pop(), self.mem.pop()
        cmdg = int(value1 != value2)
        self.mem.push(cmdg)

    def i_cmag(self):
        "Compara se maior ou igual"
        value2, value1 = self.mem.pop(), self.mem.pop()
        cmag = int(value1 >= value2)
        self.mem.push(cmag)

    def i_cmeg(self):
        "Compara se menor ou igual"
        value2, value1 = self.mem.pop(), self.mem.pop()
        cmeg = int(value1 <= value2)
        self.mem.push(cmeg)

    def i_armz(self):
        "Atribuição"
        reladdr = self.mem.pop()
        k = self.mem.pop()
        value = self.mem.pop()
        absaddr = self.mem.get(D_SEGMENT + k) + reladdr
        self.mem.set(absaddr, value)

    def i_armi(self):
        reladdr = self.mem.pop()
        k = self.mem.pop()
        value = self.mem.pop()
        ptaddr = self.mem.get(D_SEGMENT + k) + reladdr
        self.mem.set(ptaddr, value)

    def i_dsvs(self):
        "Desvia sempre"
        addr = self.mem.pop()
        self.regs.pc = addr

    def i_dsvf(self):
        "Desvia se Falso"
        addr = self.mem.pop()
        value = self.mem.pop()
        if not value:
            self.regs.pc = addr

    def i_nada(self):
        "Faz nada (no-op)"

    def i_leit(self):
        "Lê um valor da entrada padrão para a pilha"
        # TODO não está claro se o valor lido deve ser "um inteiro já
        # interpretado" ou o valor de um caractere cru (que seria o de se
        # esperar em uma implementação mais genérica)
        value = int(sys.stdin.readline())
        self.mem.push(value)

    def i_impr(self):
        "Escreve o valor da pilha para a saída padrão"
        value = self.mem.pop()
        # TODO buffers?
        sys.stdout.write(str(value) + "\n")

    def i_inpp(self):
        "Inicial execução do programa (???)"
        # TODO na descrição da especificação diz que o SP deve ser
        # inicializado em -1, o que parece um xunxo pior do que o que foi
        # feito de inicializar SP em -1 diretamente no código.
        self.mem.set(D_SEGMENT, 0)

    def i_amem(self):
        "Aloca memória (incrementa SP por uma quantidade)"
        amount = self.mem.pop()
        self.regs.sp += amount

    def i_dmem(self):
        "Desaloca memória (decrementa SP)"
        amount = self.mem.pop()
        self.regs.sp -= amount

    def i_para(self):
        "Encerra o programa"
        raise ProgramFinished, "fin"

    def i_chpr(self):
        "Chamada de procedimento (salva PC, k, e salta para endereço)"
        k = self.mem.pop()
        addr = self.mem.pop()
        self.mem.push(self.regs.pc + 1)
        self.mem.tag(self.regs.sp, "N")
        self.mem.push(self.regs.bp)
        self.mem.tag(self.regs.sp, "B")
        self.mem.push(k)
        self.mem.tag(self.regs.sp, "K")
        self.regs.pc = addr

    def i_rtpr(self):
        "Retorna de um procedimento"
        n = self.mem.pop()
        k = self.mem.pop()
        oldk = self.mem.pop()
        oldbp = self.mem.pop()
        nextpc = self.mem.pop()
        self.mem.set(D_SEGMENT + oldk, oldbp)
        sp = self.regs.sp - n
        self.regs.sp = sp
        self.regs.bp = oldbp
        self.regs.pc = nextpc

    def i_enpr(self):
        "Entra em um procedimento"
        k = self.mem.pop()
        self.regs.bp = self.regs.sp + 1
        self.mem.set(D_SEGMENT + k, self.regs.bp)

    # Instrucoes de rotulo descritas em :
    # http://tinyurl.com/descricao-instrucoes-mepa

    def i_dsvr(self):
        "Desvia para rótulo"
        # Argh!
        k = self.mem.pop()
        j = self.mem.pop()
        p = self.mem.pop()
        #while k != j:
        #    bp = self.mem.get(D_SEGMENT + k)
        #    nextbp = self.mem.get(bp-2)
        #    self.mem.set(D_SEGMENT + k, bp)
        #    k = self.mem.get(bp-1)
        bp = self.mem.get(D_SEGMENT + j)
        self.regs.bp = bp
        self.regs.pc = p
        # sp vai ser ajustado por enrt

    def i_enrt(self):
        "Entrada de rótulo"
        # Bizarro!
        n = self.mem.pop()
        k = self.mem.pop()
        self.regs.sp = self.mem.get(D_SEGMENT + k) + n - 1

    def i_cren(self):
        "Põe endereço absoluto"
        addr = self.mem.pop()
        k = self.mem.pop()
        absaddr = self.mem.get(D_SEGMENT + k) + addr
        self.mem.push(absaddr)

    def i_crvi(self):
        addr = self.mem.pop()
        k = self.mem.pop()
        absaddr = self.mem.get(D_SEGMENT + k) + addr
        refaddr = self.mem.get(absaddr)
        value = self.mem.get(refaddr)
        self.mem.push(value)

    def i_armi(self):
        addr = self.mem.pop()
        k = self.mem.pop()
        value = self.mem.pop()
        absaddr = self.mem.get(D_SEGMENT + k) + addr
        refaddr = self.mem.get(absaddr)
        self.mem.set(refaddr, value)

    # TODO:
    # - ENTR - ir para rótulo
    # - CREN - carregar endereços
    # ambos não implementados agora por estarem em estado WTF

class ReplaceByPosition:
    def __init__(self, name):
        self.name = name

class MEPA:

    def __init__(self, debug=False, tag=False):
        self.regs = RegisterSet()
        self.mem = Memory(MEMORY_SIZE, self.regs, tag=tag)
        self.instr = InstructionSet(self.regs, self.mem)
        self.debug = debug

    def execute(self, code):
        end = len(code)
        while self.regs.pc < end:
            instr, args = code[self.regs.pc]
            for arg in args:
                self.mem.push(arg)
            if self.debug:
                print "PC: 0x%x SP: 0x%x INSTR: %s ARGS: %s" % \
                      (self.regs.pc, self.regs.sp, instr.__name__, args)
            prevpc = self.regs.pc
            try:
                instr()
            except ProgramFinished:
                break
            if prevpc == self.regs.pc:
                self.regs.pc += 1

    def assemble_program(self, source):
        code = []
        labels = {}
        lines = []
        extwarns = {}
        for line in source:
            # handle comments
            if line.startswith(";"):
                continue
            found = line.find(";")
            if found != -1:
                line = line[:found]

            line = line.strip()
            index = line.find(":")
            if index == -1:
                lines.append(line)
            else:
                name, extra = line[:index].strip(), line[index+1:].strip()
                if name:
                    labels[name] = None
                    lines.append(ReplaceByPosition(name))
                if extra:
                    lines.append(extra)
        codepos = 0
        for line in lines:
            # handle comments
            if isinstance(line, ReplaceByPosition):
                labels[line.name] = codepos
                continue
            if line.startswith(";"):
                continue
            found = line.find(";")
            if found != -1:
                line = line[:found]

            line = line.strip()
            words = line.split(None, 1)
            if not words:
                continue
            name, rawargs = words[0], words[1:]
            if rawargs:
                args = rawargs[0].split(",")
            else:
                args = []
            codeargs = []
            for arg in args:
                try:
                    value = int(arg)
                except ValueError:
                    value = arg
                codeargs.append(value)
            instr = self.instr.get(name.lower())
            if getattr(instr, "extension", 0) and \
                    not extwarns.get(name.lower(), 0):
                sys.stdout.write("aviso: instrucao nao faz parte "
                        "da especificacao da MEPA: %s\n" % name)
                extwarns[name.lower()] = 1
            code.append((instr, codeargs))
            codepos += 1
        newcode = []
        for instr, codeargs in code:
            for i, arg in enumerate(codeargs[:]):
                if isinstance(arg, basestring):
                    codeargs[i] = labels[codeargs[0]]
            pair = (instr, codeargs)
            newcode.append(pair)
        return newcode

    def dump(self, output):
        self.mem.dump(output)
        self.regs.dump(output)

def parse_options():
    usage = "%prog [options] <files>"
    parser = optparse.OptionParser()
    parser.add_option("-d", "--dump", action="store_true",
                help="dumps stack and registers after execution",
                default=False)
    parser.add_option("-i", "--inspect", action="store_true",
                help="dump regs and current instruction",
                default=False)
    parser.add_option("-t", "--tagmem", action="store_true",
            help="show tags of saved pointers in memory dump",
            default=False)
    opts, args = parser.parse_args()
    return opts, args

def main(args):
    opts, args = parse_options()
    mepa = MEPA(debug=opts.inspect, tag=opts.tagmem)
    for arg in args:
        source = open(arg)
        code = mepa.assemble_program(source)
        source.close()
        mepa.execute(code)
    if opts.dump:
        mepa.dump(sys.stdout)

if __name__ == "__main__":
    main(sys.argv)
