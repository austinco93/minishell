#define STATEMENT_INITIAL_CAPACITY 80
typedef enum {W_OTHER, W_IF, W_ELSE, W_WHILE, W_END, INIT} kind_t;

typedef struct line_t{
	kind_t kind; //stmt type
	int elsend; //line index of else of end
	char *cmd; //command line
} line;

// Define a Stmt_list "object"
typedef struct stmt_list_t{
  int nstmts;      // number of added lines (always <= alines)
  int size;  // number of elements (lines) in lines array
  line *stmts;     // array of lines that we are storing
} stmt_list;

void init(stmt_list *stmt);
void add_line(stmt_list *stmt, line line);
void set_line(line *line, kind_t kind, char *cmd);
void double_capacity(stmt_list *stmt);
line get_line(stmt_list *stmt, int index);
int num_lines(stmt_list *stmt);
void set_elsend(stmt_list *stmt, int lineindex, int elsendindex);
kind_t first_word(char *line);
int read_stmt(char *line, kind_t kind, stmt_list *stmt,int infd, int outfd, int errfd);
int run_stmt(int startloc, stmt_list *stmt);
void remove_first_word(char **line);
void print_prompt(int errfd);
void free_stmt(stmt_list *stmt);