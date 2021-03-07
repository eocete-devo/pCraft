#include <stdio.h>

#include <csv.h>

#include <ami/ami.h>
#include <ami/csvread.h>

#include <ami/kvec.h>

#define CSVBUF_SIZE 1024

struct _parse_helper_t {
  int has_header;
  int index;
  char *field;
  ami_kvec_t fields;
  int current_line;
  int current_field;
  char *retfield;
};
typedef struct _parse_helper_t parse_helper_t;

void on_new_field(void *s, size_t i, void *userdata)
{
  parse_helper_t *phelp = (parse_helper_t *)userdata;
  if (!phelp) {
    fprintf(stderr, "Error reading field helper!");
    return;
  }

  if ((phelp->current_line == 1) && (phelp->has_header)) {
    kv_push(char *, phelp->fields, strdup(s));
  } else {
    if (phelp->has_header) {
      char *thips_field = kv_A(phelp->fields, phelp->current_field - 1);
      if (!strcmp(phelp->field, thips_field)) {
	if (phelp->current_line - 1 == phelp->index) {
	  phelp->retfield = strdup(s);
	}
      }
    } else {
      fprintf(stderr, "Error, only CSV with headers are supported now. Please contibute!\n");
    }
    /* if (!strcmp()) */
    /* printf("%s:%s;", kv_A(phelp->fields, phelp->current_field - 1), s); */
  }  
  
  phelp->current_field++;
}

void on_new_line(int c, void *userdata)
{
  parse_helper_t *phelp = (parse_helper_t *)userdata;
  /* printf("[%d] new line:%d\n", phelp->current_line, c); */
  phelp->current_line += 1;
  phelp->current_field = 1;
}

char *ami_csvread_get_field_at_line(ami_t *ami, char *file, int index, char *field, int has_header)
{
  FILE *fp;
  struct csv_parser p;
  char buf[CSVBUF_SIZE];
  size_t bytes_read = 0, pos = 0, retval = 0;

  parse_helper_t *phelp;  
  char *retfield = NULL;
  char *membuf = NULL;
  size_t membuf_len = 0;
  char c = 0;
  
  membuf = ami_get_membuf(ami, file);
  if (!membuf) {  
    fp = ami_get_open_file(ami, file);
    
    if (fp == NULL) {
      fp = fopen(file, "rb");
      if (!fp) {
	fprintf(stderr, "Error, could not read CSV file '%s'\n", file);
	return NULL;
      }
      /* Save the file pointer in the ami to avoid open-close too many times on the same files */
      ami_set_open_file(ami, file, fp);
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    membuf = malloc(fsize + 1);
    if (!membuf) {
      fprintf(stderr, "Could not allocate CSV buffer in memory for file %s!\n", file);
      /* fclose(fp); */
      return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    fread(membuf, 1, fsize, fp);
    membuf[fsize] = '\0';
    ami_set_membuf(ami, file, membuf);
    /* fclose(fp); */
    membuf_len = fsize + 1;
  } else {
    membuf_len = strlen(membuf) + 1;
  }

  /* printf("We read the csv file '%s' at index %d for the field '%s' and the header is set to %d\n", file, index, field, has_header); */
  
  phelp = (parse_helper_t *)malloc(sizeof(parse_helper_t));
  if (!phelp) {
    fprintf(stderr, "Error, cannot allocate parse_helper_t\n");
    return NULL;
  }

  phelp->has_header = has_header;
  phelp->index = index;
  phelp->field = field;
  kv_init(phelp->fields);
  phelp->current_line = 1;
  phelp->current_field = 1;
  phelp->retfield = NULL;
  
  csv_init(&p, CSV_APPEND_NULL);
  /* while ((bytes_read=fread(buf, 1, CSVBUF_SIZE, fp)) > 0) { */
  /* while (c = *membuf++) { */

  /* printf("membuf=%s\n", membuf); /\*  *\/ */

  char *membuf_p2;
  int cursor = 0;
  int length = 0;
  while (cursor < membuf_len) {
    membuf_p2 = &membuf[cursor];
    if ((cursor + CSVBUF_SIZE)>membuf_len) {
      length = membuf_len - cursor + 1;
    } else {
      length = CSVBUF_SIZE;
    }
    if ((retval = csv_parse(&p, membuf_p2, length, on_new_field, on_new_line, phelp)) != bytes_read) {
      if (csv_error(&p) == CSV_EPARSE) {
	      fprintf(stderr, "%s: malformed at byte %lu\n", file, (unsigned long)pos + retval + 1);
	      goto end;
      } /* else { */
      /* 	      printf("Error while processing %s: %s\n", file, csv_strerror(csv_error(&p))); */
      /* 	      goto end; */
      /* } */
    }
    if (phelp->retfield) {
      retfield = strdup(phelp->retfield);
      goto end;
    }
  /* } */
    cursor += CSVBUF_SIZE;
  }

 end:
  free(phelp->retfield);
  free(phelp);
  /* rewind(fp); */
  csv_fini(&p, NULL, NULL, NULL);
  csv_free(&p);

  return retfield;
}
