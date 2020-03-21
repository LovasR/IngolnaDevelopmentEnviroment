#include <ncurses.h> 
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int WIDTH;
int YOFFSET;
static void finish(int sig);

struct cursor_s {
    int x,y;
};
struct cursor_s* cursor;
void init_cursor(){
    cursor = (struct cursor_s*) malloc(sizeof(struct cursor_s));
    WIDTH = COLS - 2;       //Replace 2 width border
    YOFFSET = 3;            //Replace 2 with line numbering
    cursor->x = YOFFSET;
    cursor->y = 1;
}

struct line{
    int position;
    int length;
    char* text;
};

struct line* curline;

typedef struct {
    intptr_t* array;
    size_t used;
    size_t size;
    int currstart;    //holds index of line which is the first visible
} doc;

doc* main_d;

void initArray(doc* a, size_t initialSize) {
    a->array = (intptr_t*)malloc(initialSize * sizeof(intptr_t));
    a->used = 0;
    a->size = initialSize;
    a->currstart = 0;
}

void insertArray(doc *a, struct line* element) {
    if (a->used == a->size) {
        a->size += 8;
        a->array = (intptr_t*)realloc(a->array, a->size * sizeof(intptr_t));
    }
    a->array[a->used++] = (intptr_t)element;
}
void addline(struct line* line, int pos){
    if (main_d->used + 1 == main_d->size) {
        main_d->size += 8;
        main_d->array = (intptr_t*)realloc(main_d->array, main_d->size * sizeof(intptr_t));
    }
    //memmove(&main_d->array[pos], &main_d->array[(pos - 1) ], main_d->used - pos);
    for(int l = main_d->used - 1; l >= pos - 1; l--){
        main_d->array[l + 1] = main_d->array[l];
        struct line* lin = (struct line*)main_d->array[l + 1];
        lin->position += 1;
    }
    char linenum[5];
    snprintf(linenum, 5, "%d", main_d->used - 1);
    mvaddstr(1, 30, linenum);
    main_d->array[pos - 1] = (intptr_t)line;
    main_d->used++;
}

void freeArray(doc *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

int linenumlen(struct line* l){
    if(l->position >= 1000) {
        return 4;
    } else if(l->position >= 100) {
        return 3;
    } else if(l->position >= 10){
        return 2;
    } else {
        return 1;
    }
}

void renderscreen(int start){
    erase();
    main_d->currstart = start;
    int end;
    if(main_d->used >= LINES - 2){
        end = LINES - 3;
    } else {
        end = main_d->used;
        char linenum[5];
        snprintf(linenum, 5, "%d", end);
        mvaddstr(1, 15, linenum);
        snprintf(linenum, 5, "%d", start);
        mvaddstr(1, 20, linenum);
    }
    if(start > 1){
        //Checks if last line was using multiple lines and uses a "ghost" line
        addch(':');
        struct line* test = (struct line*)main_d->array[start - 1];
        if(test->length / WIDTH > 0){
            start--;
            addch(':');
        }
        test = (struct line*)main_d->array[start + end - 1];
        YOFFSET = linenumlen(test) + 1;     //+1 for space between linenum and chars
        WIDTH = COLS - YOFFSET;
    }
    int r = 0;
    for(int i = 0; i < end; i++, r++){
        move(r + 1, YOFFSET);
        struct line* lin = (struct line*)main_d->array[start + i];
        if(lin->length / WIDTH == 0){
            addstr(lin->text);
            char linenum[5];
            snprintf(linenum, 5, "%d", lin->position);
            mvaddstr(1, 25, linenum);
        } /*else { 
            int linrows = 0;        //how many rows does the line use
            if(main_d->currstart != start && i == 0){
                linrows++;
                mvaddch(0, 3, '-');
                move(r + 1, YOFFSET); 
            }
            for( ; linrows < lin->length / WIDTH; linrows++){
                char* lintext = (char*)malloc((linrows + 1) * WIDTH - linrows * WIDTH);
                memcpy(lintext, &lin->text[linrows * WIDTH], (linrows + 1) * WIDTH - linrows * WIDTH);
                addstr(lintext);
                r++;
            }
            move(r + 1, YOFFSET);                     
            char* lintext = (char*)malloc(lin->length - lin->length / WIDTH);                   //this is the last part of the line
            int partrow = (lin->length % WIDTH) * WIDTH;
            memcpy(lintext, &lin->text[linrows * WIDTH], lin->length - lin->length / WIDTH);
            addstr(lintext);
        }*/
        char linenum[5];
        snprintf(linenum, 5, "%d", lin->position);
        mvaddstr(r + 1, 0, linenum); 
    }/*
    struct line* lin = (struct line*)main_d->array[start + end];
    char linenum[5];
    snprintf(linenum, 5, "%d", lin->position);
    mvaddstr(r + 1, 0, linenum);*/
    //cursor->x = YOFFSET;
    move(cursor->y, cursor->x);
}

void up(struct line* l){
    if(l->position > 1){
        if(cursor->y > 1){
            if(l->length / (WIDTH + 1) == 0){     //WIDTH + 1 because if the line is full but there aren`t 2 lines
                struct line* upperl = (struct line*)main_d->array[l->position - 2];     //position - 2 because position starts from 1
                if(upperl->length >= l->length){
                    cursor->y -= 1;
                } else {
                    cursor->y -= 1;
                    cursor->x = upperl->length + YOFFSET;
                }
                curline = upperl;
            } else {        //this is for when the line is multi-line
                    cursor->y -= 1;
            }
        } else {
            struct line* upperl = (struct line*)main_d->array[l->position - 2];     //position - 2 because position starts from 1
            if(upperl->length < l->length) {
                cursor->x = upperl->length + YOFFSET;
            }
            curline = upperl;
            renderscreen(main_d->currstart - 1);
        }
        move(cursor->y, cursor->x);
    }
}   
void down(struct line* l){
    if(l->position < main_d->used){
        if(cursor->y < LINES - 2){
            if(l->length / (WIDTH + 1) == 0){     //WIDTH + 1 because if the line is full but there aren`t 2 lines
                struct line* downerl = (struct line*)main_d->array[l->position];     //position because position starts from 1
                if(downerl->length >= cursor->x - YOFFSET + 1){
                    cursor->y += 1;
                } else {
                    cursor->y += 1;
                    cursor->x = downerl->length + YOFFSET;
                }
                curline = downerl;
            } else {        //this is for when the line is multi-line
                //cursor->y += 1;
            }
        } else {
            struct line* downerl = (struct line*)main_d->array[l->position];     //position because position starts from 1
            if(downerl->length < l->length){
                cursor->x = downerl->length + YOFFSET;
            }
            curline = downerl;
            renderscreen(main_d->currstart + 1);
        }
        move(cursor->y, cursor->x);
    }
}
void left(struct line* l){
    if(cursor->x - YOFFSET <= 0){
        if(l->position != 1){
            cursor->y -= 1;
            if(cursor->y == 1 && l->position != 2){
                renderscreen(l->position - 1);
            }
            struct line* upperl = (struct line*)main_d->array[l->position - 2];
            curline = upperl;
            cursor->x = upperl->length + YOFFSET;
        }
    } else {
        cursor->x -= 1;
    }
    int chpos = cursor->x - YOFFSET;       /*+ WIDTH * (l->length / WIDTH) */
    char linenum[5];
    snprintf(linenum, 5, "%d", chpos);
    mvaddstr(3, 10, linenum);
    move(cursor->y, cursor->x);
}
void right(struct line* l){
    if(cursor->x - YOFFSET == l->length){    //TODO make it support multiline
        if(l->position < main_d->used){
            if(cursor->y == LINES - 1){
                renderscreen(main_d->currstart + 1);
            }
            cursor->y += 1;
            struct line* downerl = (struct line*)main_d->array[l->position];
            curline = downerl;
            cursor->x = YOFFSET;
        }
    } else {
        cursor->x += 1;
    }
    move(cursor->y, cursor->x);
}

void addchar(struct line* l, char* ch){
    //TODO determine position to insert to based on cursor->x
    int chpos = cursor->x - YOFFSET;       /*+ WIDTH * (l->length / WIDTH) */
    char linenum[5];
    snprintf(linenum, 5, "%d", chpos);
    mvaddstr(3, 10, linenum);
    if(chpos != l->length){
        char* tmptext = (char*)memset(malloc(l->length + 1), 0, l->length + 1);
        memmove(tmptext, l->text, chpos);
        mvaddch(3, 15, *ch);
        strcat(tmptext, ch);
        memmove(&tmptext[chpos + 1], &l->text[chpos], l->length - chpos + 1);
        memmove(l->text, tmptext, l->length + 1);
        free(tmptext);
    } else {
        strcat(l->text, ch);
    }
    if(cursor->x == COLS - 1){
        cursor->x = YOFFSET;
        cursor->y += 1;
        mvaddch(cursor->y, 0, '|');
        if(cursor->y == LINES - 2){
            renderscreen(l->position);
        }
    } else {
    }
    mvaddstr(cursor->y, YOFFSET, l->text);
    //mvaddch(cursor->y, cursor->x, (char) *(l->text + l->length));
    l->length += 1;
    if(l->length % 128 == 0){
        addstr("!");
        l->text = (char*)realloc(l->text, l->length + 128);
    }
    
    
    //char linenum[4];
    snprintf(linenum, 4, "%d", l->position);
    mvaddstr(cursor->y, 0, linenum);
    
    
    cursor->x += 1;
    move(cursor->y, cursor->x);
}
void removeline(struct line* l){
    if(main_d->used > 2){
    for(int lin = l->position; lin < main_d->used; lin++){
        main_d->array[lin - 1] = main_d->array[lin];
        struct line* ln = (struct line*)main_d->array[lin - 1];
        ln->position -= 1;
    }
    }
    main_d->used -= 1;
    main_d->array[main_d-> used] = 0;
    free(l->text);
    free(l);
}
void removechar(struct line* l){
    if(l->length > 0){
        l->length -= 1;
        cursor->x -= 1;
        if(cursor->x - YOFFSET < l->length){
            for(int pos = cursor->x - YOFFSET + 1; pos < l->length; pos++){
                l->text[pos - 1] = l->text[pos];
            }
        }
        l->text[l->length] = 0;
        /*char* e = l->text + l->length;
        *e = 0;*/
        //mvaddch(cursor->y, cursor->x, 32);
        renderscreen(main_d->currstart);
    } else {
        if(l->position > 1){
            curline = (struct line*)main_d->array[l->position - 2];
            removeline(l);
            if(main_d->used > 1){
                struct line* upperl = (struct line*)main_d->array[curline->position - 2];
                cursor->x = upperl->length + YOFFSET;
            } else {
                cursor->x = YOFFSET + 1;
            }
            if(main_d->used > LINES - 1){
                renderscreen(main_d->currstart - 1);
            } else {
                cursor->y -= 1;
                renderscreen(main_d->currstart);
            }                
        }
    }
}
struct line* newline(int pos){
    struct line* line_n = (struct line*) memset(malloc(sizeof(struct line)), 0, sizeof(struct line));
    line_n->text = (char*) memset(malloc(128), 0, 128);       //change 32 if needed
    line_n->length = 0;
    line_n->position = pos;
    if(line_n->position == main_d->used + 1){
        insertArray(main_d, line_n);
    } else {
        addch(':');
        addline(line_n, pos);
        renderscreen(main_d->currstart);
    }
    return line_n;
}
struct line* enter(struct line* l){
    cursor->x = YOFFSET;
    struct line* line_n = newline(l->position + 1);
    if(cursor->y == LINES - 2){
        renderscreen(main_d->currstart + 1);
    } else {
        cursor->y += 1;
    }
    char linenum[5];
    snprintf(linenum, 5, "%d", line_n->position);
    mvaddstr(cursor->y, 0, linenum);
    move(cursor->y, cursor->x);
    return line_n;
}
void readfile(char* path){
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(path, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    int r = 1;
    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", line);
        curline = newline(r);
        curline->length = read - 1;
        curline->text = memmove(malloc(read - 1), line, read - 1);
        
        char linenum[5];
        snprintf(linenum, 5, "%d", read);
        mvaddstr(2, 30, linenum);
        r++;
    }

    fclose(fp);
    if (line)
        free(line);
    curline = (struct line*)main_d->array[0];
}
void savefile(char* path){
    FILE * fp;
    fp = fopen(path, "w");
    for(int i = 0; i < main_d->used; i++){
        struct line* l = (struct line*)main_d->array[i];
        char* outext = l->text;
        outext[l->length] = '\n';
        fprintf(fp, "%s", outext);
    }
    fclose(fp);
    finish(0);
}
int main(int argc, char* argv[]){
    signal(SIGINT, finish);
    initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();
    noecho();
    
    if (has_colors())
    {
        start_color();

        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_CYAN,    COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }
    
    
    init_cursor();      // must be first to init
    
    //attron(A_UNDERLINE);
    //wborder(stdscr, 0, 0, 0, 0, 0, 0, 0, 0);
    main_d = (doc*)malloc(sizeof(doc));
    initArray(main_d, 12);
    init_cursor();
    if(argc != 1){
        readfile(argv[1]); 
        renderscreen(main_d->currstart);
    } else {
        curline = newline(1);
    }
    renderscreen(main_d->currstart);
    int i = 0;
    for ( ; ; ){
        int c = getch();
        switch(c){
            case KEY_BACKSPACE:
                removechar(curline);
                break;
            case '\r'   /*KEY_HOME*/:
                curline = enter(curline);
                //move(cursor->x, cursor->y);
                break;
            case KEY_UP:
                up(curline);
                break;
            case KEY_DOWN:
                down(curline);
                break;
            case KEY_LEFT:
                //addch('!');
                left(curline);
                break;
            case KEY_RIGHT:
                //addch('!');
                right(curline);
                break;
            case KEY_END:
                savefile(argv[1]);
                break;
            default:
                addchar(curline, (char*) &c);\
                break;
        }
        //attrset(COLOR_PAIR(i % 8));
        i++;
    } 
    finish(0);           
}

static void finish(int sig)
{
    endwin();

    exit(0);
}
