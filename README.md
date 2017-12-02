Команда ls-clone выводит список всех файлов и каталогов текущего каталога. Это клон утилиты ls linux, который поддерживает только единственную опцию без ключей.

Результаты печатаются на стандартный вывод, по одному файлу на строку, вместе с флагами файла, размером, владальцем и тд.

Создать:
	git clone https://github.com/Vecnik88/ls-clone.git
	make
	sudo make install

Пример вывода команды:
	ls-clone
	drwxrwxr-x 3  anton    anton       4096 2017-12-02 15:49 .
	drwxr-xr-x 71 anton    anton       4096 2017-12-02 15:38 ..
	-rw-rw-r-- 1  anton    anton       1055 2017-12-02 15:43 LICENSE
	-rwxrwxr-x 1  anton    anton      79256 2017-12-02 15:46 ls-clone
	-rw-rw-r-- 1  anton    anton        465 2017-12-02 15:40 Makefile
	-rw-rw-r-- 1  anton    anton        556 2017-12-02 15:50 README.md
	drwxrwxr-x 2  anton    anton       4096 2017-12-02 15:46 src
