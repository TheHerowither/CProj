#include <stdio.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CLOG_IMPLEMENTATION
#include <clog.h>

#include "cJSON.h"


const ClogLevel JSON_ERROR = CLOG_REGISTER_LEVEL("Json error", CLOG_COLOR_RED, 3);
const ClogLevel COMMAND    = CLOG_REGISTER_LEVEL("COMMAND", CLOG_COLOR_FAINT, 2);

typedef enum {
	MODE_NONE = 0,
	MODE_NEW,
	MODE_BUILD,
	MODE_ADD,
} Mode;


const char *read_file(const char *path) {
  	FILE *file = fopen(path, "r");
  	if (file == NULL) {
    	fprintf(stderr, "Expected file \"%s\" not found", path);
    	return NULL;
  	}
  	fseek(file, 0, SEEK_END);
  	long len = ftell(file);
  	fseek(file, 0, SEEK_SET);
  	char *buffer = malloc(len + 1);

  	if (buffer == NULL) {
    	fprintf(stderr, "Unable to allocate memory for file");
    	fclose(file);
    	return NULL;
  	}

  	fread(buffer, 1, len, file);
  	buffer[len] = '\0';

  	return (const char *)buffer;
}

void usage(const char *program, Mode mode) {
	if (mode == MODE_NONE) printf("[USAGE]: %s <mode>\n", program);
	if (mode == MODE_NEW)  printf("[USAGE]: %s new <name>\n", program);
}

Mode get_mode_from_string(const char *mode_str) {
	if (strcmp(mode_str, "new") == 0) 		 return MODE_NEW;
	else if (strcmp(mode_str, "build") == 0) return MODE_BUILD;
	else if (strcmp(mode_str, "add") == 0)   return MODE_ADD;

	else {
		clog(CLOG_ERROR, "Invalid mode supplied!");
		exit(1);
	}
}

void new_project(int argc, char **argv) {
	if (argc < 3) {
		clog(CLOG_ERROR, "Not enough argument supplied!");
		usage(argv[0], MODE_NEW);
		exit(1);
	}

	const char *name = argv[2];
	char *cwd = malloc(1024);
	getcwd(cwd, 1024);

	clog(CLOG_TRACE, "Creating new project %s", name);

	char *path = malloc(1024);
	strcpy(path, cwd);
	strcat(path, "/");
	strcat(path, name);

	clog(CLOG_DEBUG, "Creating directory %s", path);
	
	struct stat st = {0};
	if (stat(path, &st) == -1) {
		mkdir(path, 0777);
	}
	else {
		clog(CLOG_ERROR, "Folder %s already exists", path);
		free(cwd);
		free(path);
		exit(1);
	}

	clog(CLOG_DEBUG, "chdir %s", path);
	chdir(path);

	cJSON *config_j = cJSON_CreateObject();
	cJSON *files = cJSON_CreateArray();
	cJSON_AddStringToObject(config_j, "name", name);
	cJSON_AddStringToObject(config_j, "cc", "gcc");
	cJSON_AddItemToObject(config_j, "files", files);
	cJSON_AddItemToArray(files, cJSON_CreateString("main.c"));

	FILE *config_f = fopen("cproj.json", "w");
	fputs(cJSON_Print(config_j), config_f);
	fclose(config_f);

	FILE *main_c = fopen("main.c", "w");
	fputs("#include <stdio.h>\n\n\n\n\n"
	   	  "int main() {\n"
	      "    printf(\"Hello, World!\\n\");\n"
	   	  "    return 0;\n"
	      "}",
	   main_c);
	fclose(main_c);
	
	cJSON_Delete(config_j);
	free(cwd);
	free(path);
}

void build_project(void) {
	char *path = malloc(1024);
	char *cwd = malloc(1024);
	char *files_str = malloc(1024);
	getcwd(path, 1024);
	getcwd(cwd, 1024);
	strcat(path, "/cproj.json");

	if (access(path, F_OK) != 0) {
		clog(CLOG_ERROR, "Directory is not a valid cproj directory");
	}
	
	const char *config_str = read_file(path);
	cJSON *config = cJSON_Parse(config_str);

	cJSON *name  = cJSON_GetObjectItemCaseSensitive(config, "name");
	cJSON *cc    = cJSON_GetObjectItemCaseSensitive(config, "cc");
	cJSON *files = cJSON_GetObjectItemCaseSensitive(config, "files");

	for (int i = 0; i < cJSON_GetArraySize(files); i++) {
		cJSON *file = cJSON_GetArrayItem(files, i);
		strcat(files_str, file->valuestring);
		strcat(files_str, " ");
	}
	
	struct stat st = {0};
	if (stat(strcat(cwd, "/bin"), &st) == -1) mkdir(cwd, 0777);
	char *cmd = malloc(1024);
	sprintf(cmd, "%s %s -o \"%s/%s\"", cc->valuestring, files_str, cwd, name->valuestring);
	clog(COMMAND, "%s", cmd);
	system(cmd);

	cJSON_Delete(config);
	free(path);
	free(cwd);
	free(files_str);
	free(cmd);
}

void add_to_project(int argc, char **argv) {
	(void)argc;
	(void)argv;
}

int main(int argc, char **argv) {
	clog_set_fmt("%c[%L]:%r %m");
	if (argc < 2) {
		clog(CLOG_ERROR, "Not enough arguments supplied!");
		usage(argv[0], MODE_NONE);
		return 1;
	}
	const char *mode_str = argv[1];
	Mode mode = get_mode_from_string(mode_str);

	if 		(mode == MODE_NEW) 	 new_project(argc, argv);
	else if (mode == MODE_BUILD) build_project();
	else if (mode == MODE_ADD)   add_to_project(argc, argv);

	return 0;
}
