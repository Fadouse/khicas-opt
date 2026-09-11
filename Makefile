.PHONY: all optimized official test clean

all: optimized

optimized:
	python3 tools/build.py optimized

official:
	python3 tools/build.py official

test:
	$(if $(TEST),,$(error Specify the affected test: make test TEST=tests/run-name.py))
	python3 $(TEST) $(TEST_ARGS)

clean:
	python3 -c "import shutil; shutil.rmtree('.build/optimized', ignore_errors=True); shutil.rmtree('.build/official', ignore_errors=True)"
