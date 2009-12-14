#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef WIN32
#define PATH_MAX BUFSIZ
#define random rand
#warning use um sistema operacional que presta
#endif

static int verbosidade = 0;

void debugf(int nivel, const char *fmt, ...)
{
	va_list ap;

	if (nivel <= verbosidade) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
	}
}

typedef unsigned int gene_t;

struct cromossomo {
	gene_t *genes;
	float fitness;
};

struct arquivo {
	char caminho[PATH_MAX];
	size_t caminho_tam;
	off_t tamanho;

	struct arquivo *prox;
};

/* contexto -- estrutura tambem conhecida como "monte de variaveis que
 * poderiam ser globais" */
struct contexto {	
	size_t geracao_atual;

	off_t tamanho_midia;
	off_t tamanho_total;
	int ideal;

	int tamanho_populacao;

	size_t ngenes;

	char *caminho;
	struct arquivo **arquivos;
	size_t narquivos;
	int mostra_arquivos;

	int p_espaco;  /* espaco usado para calculo das probabilidades de
			  mudacao e recomb */

	int p_mutacao; /* prob. de mutacao (* 1/p_espaco) */
	int p_mutarao; /* quantos genes mutarao durante mutacao */

	int corte_selecao; /* quantos serao cortados na selecao */

	int p_recombinacao;
	int recombinacao_area;

	int ngeracoes;
	float *geracoes_medias; /* mantem dados de medias por geracao */

	int verbosidade;
};

void *ealloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr) {
		fputs("falhou pra alocar memoria!\n", stderr);
		exit(2);
	}

	return ptr;
}

void erro(char *msg)
{
	fprintf(stderr, msg);
	exit(1);
}

struct cromossomo *novo_cromossomo(size_t ngenes)
{
	struct cromossomo *novo;

	novo = (struct cromossomo*) ealloc(sizeof(struct cromossomo));
	novo->genes = (gene_t*) ealloc(sizeof(gene_t) * ngenes);
	novo->fitness = 0.0;
	return novo;
}

struct arquivo **lista_arquivos(const char *basedir, size_t *narqs)
{
	DIR *d;
	struct dirent *dir;
	struct stat st;
	struct arquivo *atual, *ant;
	struct arquivo *arquivos = NULL;
	struct arquivo **arquivos_array;
	size_t i;
	
	*narqs = 0;

	d = opendir(basedir);
	if (!d) {
		perror("erro ao listar diretorio");
		exit(1);
	}

	while ((dir = readdir(d))) {
		atual = (struct arquivo*) ealloc(sizeof(struct arquivo));
		atual->prox = NULL;

		strcpy(atual->caminho, basedir);
		strcat(atual->caminho, "/");
		strcat(atual->caminho, dir->d_name);

		if (stat(atual->caminho, &st) < 0) {
			perror(dir->d_name);
			exit(1);
		}
		atual->tamanho = st.st_size;
		if (!arquivos)
			arquivos = ant = atual;
		else {
			ant->prox = atual;
			ant = atual;
		}

		*narqs += 1;
	}

	closedir(d);

	/* aloca os arquivos em um array, pois eles serao referenciados
	 * pelos seus indices nos cromossomos: */

	arquivos_array = (struct arquivo**) ealloc(sizeof(struct arquivo*) * *narqs);
	atual = arquivos;
	
	i = 0;
	while (atual) {
		arquivos_array[i++] = atual;
		atual = atual->prox;
	}

	return arquivos_array;
}

struct cromossomo **cria_populacao(size_t individuos, size_t genes)
{
	size_t i;
	struct cromossomo **pop;

	pop = (struct cromossomo**) ealloc(sizeof(struct cromossomo*) * individuos);

	for (i = 0; i < individuos; i++)
		pop[i] = novo_cromossomo(genes);

	return pop;
}

void shuffle(gene_t *genes, size_t ngenes, int voltas)
{
	gene_t tmp;
	size_t i, volta;
	size_t de, para;

	for (volta = 0; volta < voltas; volta++) {
		for (i = 0; i < ngenes; i++) {
			de = i;
			para = random() % ngenes;

			tmp = genes[de];
			genes[de] = genes[para];
			genes[para] = tmp;
		}
	}
}

void gera_individuo(struct cromossomo *cromossomo, size_t ngenes)
{
	size_t i;

	for (i = 0; i < ngenes; i++)
		cromossomo->genes[i] = (gene_t) i;
	shuffle(cromossomo->genes, ngenes, 1000);
}

void mostra_individuo(struct cromossomo *individuo, size_t ngenes)
{
	size_t i;

	debugf(2, "(%05f)", individuo->fitness);
	for (i = 0; i < ngenes; i++)
		debugf(2, "%04d ", individuo->genes[i]);
	debugf(2, "\n");
}

float fitness(struct cromossomo *individuo, struct contexto *ctx)
{
	float fitness;
	size_t i;
	size_t idx;
	size_t usado = 0;
	off_t tamanho;
	int nmidias = 1;

	for (i = 0; i < ctx->ngenes; i++) {
		idx = individuo->genes[i];
		//assert(idx < ctx->narquivos);
		tamanho = ctx->arquivos[idx]->tamanho;
		if (tamanho > ctx->tamanho_midia) {
			/* preenche a midia atual com o que for possivel */
			tamanho -= ctx->tamanho_midia - usado;
			++nmidias;
			/* consome o numero de midias necessario */
			nmidias += tamanho / ctx->tamanho_midia;
			/* e deixa o resto para ir com outros arquivos */
			usado = tamanho % ctx->tamanho_midia;
			tamanho = 0;
		}
		else if (usado + tamanho > ctx->tamanho_midia) {
			nmidias++;
			usado = tamanho;
		}
		usado += tamanho;
	}
	fitness = (float)nmidias - (float)ctx->ideal;
	//debugf(2, "fit %x: %d - %d = %.2f\n", individuo, nmidias, ctx->ideal, fitness);

	return fitness;
}

int fitness_cmp(const void *um, const void *outro)
{
#define DEREFCR(ptr) (*(struct cromossomo**)ptr)

	if (DEREFCR(um)->fitness > DEREFCR(outro)->fitness)
		return 1;
	else if (DEREFCR(um)->fitness == DEREFCR(outro)->fitness)
		return 0;
	else
		return -1;
}

void fitness_populacao(struct cromossomo **populacao, size_t individuos, struct contexto *ctx)
{
	size_t i;
	float total = 0.0;

	for (i = 0; i < individuos; i++)
		total += populacao[i]->fitness = fitness(populacao[i], ctx);

	qsort(populacao, individuos, sizeof(struct cromossomo*), fitness_cmp);

	debugf(2, "fitness [");
	for (i = 0; i < individuos; i++)
		debugf(2, "%d:%.2f ", i, populacao[i]->fitness);
	debugf(2, "]\n");
	ctx->geracoes_medias[ctx->geracao_atual] = total / individuos;
	//debugf(1, "fitness media: %.2f\n", total / individuos);
}

void mutacao(struct cromossomo **populacao, size_t individuos, struct contexto *ctx)
{	
	size_t i, imut, igene1, igene2;
	size_t mutarao;
	gene_t tmp;

	mutarao = ctx->ngenes / ctx->p_mutarao; /* 1/p_mutarao dos genes mutarao */
	if (mutarao == 0)
		mutarao = 1;

	debugf(2, "mutacoes (%d genes): [", mutarao);
	for (i = 0; i < individuos; i++) {
		if (random() % ctx->p_espaco <= ctx->p_mutacao) {
			/* ok, deu sorte, mutara */
			debugf(2, "%d ", i);
			/* shuffle nao pode ser usado aqui, por iterar em ordem
			 * definida */
			for (imut = 0; imut < mutarao; imut++) {
				igene1 = random() % ctx->ngenes;
				igene2 = random() % ctx->ngenes;
				tmp = populacao[i]->genes[igene1];
				populacao[i]->genes[igene1] = populacao[i]->genes[igene2];
				populacao[i]->genes[igene2] = tmp;
			}
		}
	}
	debugf(2, "]\n");
}

void cruza(struct cromossomo *joao, struct cromossomo *maria,
		struct cromossomo *filho, struct contexto *ctx)
{
	size_t igene, igjoao, ate;
	gene_t tmp;

	/* esse cruzamento copia todos os genes de joao; escolhe alguma
	 * regiao (de tamanho recombinacao_area) do cromossomo de maria e
	 * copia os cromossomos de maria para o filho;
	 *
	 * o que complica essa copia eh que eh necessario reorganizar os
	 * genes do filho, para que nao hajam repeticoes na disposicao. */
	memcpy(maria->genes, joao->genes, sizeof(gene_t) * ctx->ngenes);
	igene = random() % (ctx->ngenes - ctx->recombinacao_area);
	ate = igene + ctx->recombinacao_area;
	for (; igene < ate; igene++) {	
		tmp = maria->genes[igene];
		for (igjoao = 0; igjoao < ctx->ngenes; igjoao++)
			if (tmp == joao->genes[igjoao])
				break;
			/* sem condicao de erro, ele sempre deve achar */
		filho->genes[igene] = tmp;
		filho->genes[igjoao] = joao->genes[igene];
	}
}

void crossover(struct cromossomo **populacao, size_t individuos, struct contexto *ctx)
{
	size_t i;
	struct cromossomo **antiga;

	antiga = (struct cromossomo**) ealloc(sizeof(struct cromossomo*) * individuos);
	memcpy(antiga, populacao, sizeof(struct cromossomo*) * individuos);

	const size_t iteracoes = individuos - 1;
	for (i = 0; i < iteracoes; i += 2) {
		cruza(antiga[i], antiga[i+1], populacao[i], ctx);
		cruza(antiga[i+1], antiga[i], populacao[i+1], ctx);
	}

	free(antiga);
}

/** selecao
 *
 * Faz a selecao dos individuos "mais aptos". Utiliza o "metodo da roleta",
 * que eh amplamente descrito na literatura. Blah.
 *
 * Utiliza corte_selecao, para elimitar os N menos aptos da populacao.
 */
void selecao(struct cromossomo **populacao, size_t individuos, struct contexto *ctx)
{
	size_t i, j, c, selecionado;
	size_t igene;
	float maior = 0;
	float menor = -1;
	float fittotal = 0.0;
	int total = 0;
	int *quantos;
	int *roleta;
	struct cromossomo **antiga; /* FIXME esses arrays podem ser
				       alocados no inicio do programa, pois
				       nunca mudam de tamanho */
	/* coleta maior e menor fitness */
	for (i = 0; i < individuos; i++) {
		if (populacao[i]->fitness > maior)
			maior = populacao[i]->fitness;
		if (populacao[i]->fitness < menor || menor == -1)
			menor = populacao[i]->fitness;
		fittotal += populacao[i]->fitness;
	}

	quantos = (int*) ealloc(sizeof(int) * individuos);

	/* conta quantos elementos serao necessarios na roleta */
	total = 0;
	size_t slots = individuos;
	if (ctx->corte_selecao >= individuos) {
		fprintf(stderr, "aviso: corte de selecao %d maior que "
				"populacao %d, ignorando corte\n", ctx->corte_selecao,
				individuos);
		ctx->corte_selecao = 0;
	}
	size_t sorteados = individuos - ctx->corte_selecao;
	debugf(2, "roleta: [");
	for (i = 0; i < sorteados; i++) {
		total += quantos[i] = slots--;
		debugf(2, "%d:%d ", i, quantos[i]);
	}
	debugf(2, "]\n");

	/* prepara a tal roleta */
	roleta = (int*) ealloc(sizeof(int) * total);
	for (i = 0, c = 0; i < sorteados; i++)
		for (j = 0; j < quantos[i]; j++)
			roleta[c++] = i;

	/* copia os cromossomos */
	antiga = (struct cromossomo**) ealloc(sizeof(struct cromossomo*) * individuos);
	memcpy(antiga, populacao, sizeof(struct cromossomo*) * individuos);

	/* realiza o grande sorteio */
	debugf(2, "selecionados: [");
	for (i = 0; i < individuos; i++) {
		c = random() % total;
		selecionado = roleta[c];
		debugf(2, "%d:%d ", i, selecionado);
		/* copia genes e fitness de um para outro */
		for (igene = 0; igene < ctx->ngenes; igene++)
			populacao[i]->genes[igene] = antiga[selecionado]->genes[igene];
		populacao[i]->fitness = antiga[selecionado]->fitness;
	}
	debugf(2, "]\n");

	free(quantos);
	free(roleta);
	free(antiga);
}

void mostra(struct cromossomo **populacao, struct contexto *ctx)
{
	size_t i, idx;

	for (i = 0; i < ctx->narquivos; i++) {
		idx = populacao[0]->genes[i];
		puts(ctx->arquivos[idx]->caminho);
	}
}

void aloca_estatisticas(struct contexto *ctx)
{
	ctx->geracoes_medias = ealloc(sizeof(ctx->geracoes_medias[0]) * ctx->ngeracoes);
}

void mostra_medias(struct contexto *ctx)
{
	size_t i;

	for (i = 0; i < ctx->ngeracoes; i++) {
		debugf(1, "geracao: %u\n", i);
		debugf(1, "fitness media: %.2f\n", ctx->geracoes_medias[i]);
	}
}

struct cromossomo **pensa(struct contexto *ctx)
{
	size_t i;

	struct cromossomo **popatual =
		cria_populacao(ctx->tamanho_populacao, ctx->narquivos);

	for (i = 0; i < ctx->tamanho_populacao; i++)
		gera_individuo(popatual[i], ctx->ngenes);
	fitness_populacao(popatual, ctx->tamanho_populacao, ctx);

	for (ctx->geracao_atual = 0; ctx->geracao_atual < ctx->ngeracoes;
			ctx->geracao_atual++) {
		//debugf(1, "geracao: %d\n", i);
		//for (j = 0; j < ctx->tamanho_populacao; j++)
		//	mostra_individuo(popatual[j], ctx->ngenes);
		selecao(popatual, ctx->tamanho_populacao, ctx);
		mutacao(popatual, ctx->tamanho_populacao, ctx);
		fitness_populacao(popatual, ctx->tamanho_populacao, ctx);
		crossover(popatual, ctx->tamanho_populacao, ctx);
	}

	return popatual;
}

void ajuda()
{
	debugf(1, "programa [opcoes] <diretorio>\n"
		"Opcoes: \n"
		"\n"
		"	-p POPULACAO\n"
		"	-E ESPACO para os valores de mut e recomb\n"
		"	-m PROB para mutacao (PROB/ESPACO)\n"
		"	-M MUTARAO proporcao de genes mutados\n"
		"	-r PROB para recombinacao (PROB/ESPACO)\n"
		"	-R AREA num. de genes contiguos herdados\n"
		"	-c CORTE quantos serao cortados na selecao\n"
		"	-g GERACOES num. de geracoes para execucao\n"
		"	-d 0/1 1 para mostrar arquivos no final da exec.\n"
		"	-s TAMANHO da midia, em bytes\n"
		"	-v NIVEL verbosidade 0=nada 1=medias 2=tudo\n"
		"	-h AAJUUUUDA!\n"
		"\n");
	exit(0);
}

void avalia_opcoes(int argc, char *argv[], struct contexto *ctx)
{
	size_t i, iopt;
	struct {
		char opt;
		int *dest;
	} opts[] = {
		{'p', &ctx->tamanho_populacao},
		{'E', &ctx->p_espaco},
		{'m', &ctx->p_mutacao},
		{'M', &ctx->p_mutarao},
		{'r', &ctx->p_recombinacao},
		{'R', &ctx->recombinacao_area},
		{'c', &ctx->corte_selecao},
		{'g', &ctx->ngeracoes},
		{'d', &ctx->mostra_arquivos},
		{'v', &ctx->verbosidade},
		{'s', (int*)&ctx->tamanho_midia}
	};

	ctx->caminho = NULL;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			ctx->caminho = argv[i];
		else {
			if (argv[i][1] == 'h')
				ajuda();
			if (i + 1 == argc) {
				fprintf(stderr, "eita, falta argumento para %s\n", argv[i]);
				exit(1);
			}
			const size_t optscount = sizeof(opts) / sizeof(opts[0]);
			for (iopt = 0; iopt < optscount; iopt++)
				if (argv[i][1] == opts[iopt].opt) {
					*opts[iopt].dest = atoi(argv[++i]);
					break;
				}
			if (iopt == optscount) {
				fprintf(stderr, "opcao invalida: %s\n", argv[i]);
				exit(1);
			}
		}
	}

	if (!ctx->caminho)
		ajuda();
}

int main(int argc, char *argv[])
{
	struct contexto ctx;
	size_t i;
	int ret = 0;

	ctx.verbosidade = 1;
	ctx.mostra_arquivos = 0;
	ctx.tamanho_midia = 700 * 1024;
	ctx.tamanho_total = 0;
	ctx.tamanho_populacao = 10;
	ctx.corte_selecao = ctx.tamanho_populacao / 2;
	ctx.p_espaco = 100;
	ctx.p_mutarao = 20;
	ctx.p_mutacao = 1;
	ctx.p_recombinacao = 8;
	ctx.ngeracoes = 10000;
	ctx.recombinacao_area = 10;
	avalia_opcoes(argc, argv, &ctx);
	aloca_estatisticas(&ctx);
	verbosidade = ctx.verbosidade; /* blarg */
	ctx.arquivos = lista_arquivos(ctx.caminho, &ctx.narquivos);
	ctx.ngenes = ctx.narquivos;

	for (i = 0; i < ctx.narquivos; i++)
		ctx.tamanho_total += ctx.arquivos[i]->tamanho;
	ctx.ideal = ctx.tamanho_total / ctx.tamanho_midia; /* FIXME signedness */
	if (ctx.ideal == 0)
		ctx.ideal = 1;
	debugf(2, "numero de arquivos: %u\n", ctx.narquivos);
	debugf(2, "tamanho total: %u\n", ctx.tamanho_total);
	debugf(2, "tamanho de midia: %u\n", ctx.tamanho_midia);
	debugf(2, "numero ideal de midias: %d\n", ctx.ideal);
	debugf(2, "espaco de p: %d\n", ctx.p_espaco);
	debugf(2, "populacao: %d\n", ctx.tamanho_populacao);
	debugf(2, "corte de selecao: %d\n", ctx.corte_selecao);
	debugf(2, "p. de mutacao: %d\n", ctx.p_mutacao);
	debugf(2, "quantos genes mutacao: %d\n", ctx.p_mutarao);
	debugf(2, "p. de recomb: %d\n", ctx.p_recombinacao);
	debugf(2, "numero de geracoes: %d\n", ctx.ngeracoes);

	srandom((unsigned int)time(NULL));

	if (ctx.tamanho_total < ctx.tamanho_midia) {
		debugf(2, "os arquivos ocupam menos espaco que o tamanho da midia\n");
		debugf(2, "nao ha o que ser feito\n");
	}
	else {
		struct cromossomo **pop;
		pop = pensa(&ctx);
		mostra_medias(&ctx);
		if (ctx.mostra_arquivos)
			mostra(pop, &ctx);
	}

	return ret;
}

