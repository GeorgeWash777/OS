all: libImpl1.so libImpl2.so progone progtwo

# сборка lib1
libImpl1.so: prime_naive.o gcd_evclid.o
	gcc -shared -o $@ $^ -lm

# сборка lib2
libImpl2.so: prime_eratosthenus.o gcd_naive.o
	gcc -shared -o $@ $^ -lm

# компиляция lib1
prime_naive.o: prime_naive.c
	gcc -fPIC -Wall -Wextra -c $< -o $@

gcd_evclid.o: gcd_evclid.c
	gcc -fPIC -Wall -Wextra -c $< -o $@

# компиляция lib2
prime_eratosthenus.o: prime_eratosthenus.c
	gcc -fPIC -Wall -Wextra -c $< -o $@

gcd_naive.o: gcd_naive.c
	gcc -fPIC -Wall -Wextra -c $< -o $@

# сборка первой проги
progone: progone.c
	gcc -o $@ $< -L. -lImpl1 -lm -Wl,-rpath,.

# Сборка тестовой программы №2 (динамическая загрузка библиотек)
progtwo: progtwo.c
	gcc -o $@ $< -ldl

# Очистка сгенерированных файлов
clean:
	rm -f *.o *.so progone progtwo

# Указание целей, которые не являются файлами
.PHONY: all clean