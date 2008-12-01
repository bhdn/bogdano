CFLAGS = -g -Wall
all: tokenize toscal run-tests
tokenize: tokenize.o input.o test-tokenize.o
toscal: tokenize.o input.o parser.o toscal.o symbols.o type.o hash.o \
	semantic.o string_list.o parameters.o codegen.o
test:
	./run-tests
	./run-tests-lexer.py
	./run-tests-semantic.py
	./run-tests-codegen.py
update-tests: update-tests-tokenizer update-tests-parser update-tests-semantic
update-tests-tokenizer: tokenize
	for test in tests/tokenizer/success/*.txt tests/tokenizer/fail/*.txt; do \
		./tokenize $$test  &> $$test-output || :; \
		done;
update-tests-parser: toscal
	for test in tests/parser/success/*.txt tests/parser/fail/*.txt; do \
		./toscal -d -S < $$test &> $$test-output || :; \
		done;
update-tests-semantic: toscal
	for test in tests/semantic/success/*.pas tests/semantic/fail/*.pas; do \
		./toscal -C < $$test &> $$test-output || :; \
		done;
update-tests-codegen: toscal
	for test in tests/codegen-mepa/success/*.pas tests/codegen-mepa/fail/*.pas; do \
		./toscal -W < $$test &> $$test-output || :; \
		done;
%.o: %.h
