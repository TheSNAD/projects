#include "shell.h"
/* обработка строк, основные встроенные команды, обращения к функциям в других частях через shell.h */

int c = '\n';
int cash_c;
char *token = NULL;
char *buf;
char *buf_buf;
int condition = IN_PROG;
int conveyor = 0;
int error = 0;
int last_condition = 0;
struct job curr_job;
struct program *cash_programs;
char **cash_arguments;
char *cash_str;
char **history = NULL;
int hist = 0;
int thist = 0;
int hist_size = 0;
int hist_curr = 0;
int hist_rec = 0;
int cash = 0;
/* аргументы шелла берем из mainshell.c */
extern int margc;
extern char **margv;

/*     ОБРАБОТЧИК КОМАНД (внутренняя обработка) */
void getsymbol()
{
    token = NULL;
    /* пропускаем первые пробелы/табы, но заносим в историю */
    while((c == ' ') || (c == '\t'))
    {
        c = mgetchar();
    }
    
    /* если !ЧИСЛО */
    if(c == '!')
    {
        /* (thist сохраняет номер команды) */
        thist = 0;
        cash = 1;
        c = mgetchar();
        
        if(isspecsymbol(c) == 0)
        {
            cash++;
            /* сначала считываем само ЧИСЛО */
            while( (c >= '0') && (c <= '9') )
            {
                thist *= 10;
                thist += c - '0';
                /*(cash отсчитывает cимволы в введенной строке)*/
                cash++;
                c = mgetchar();
            }
            /* если после числа идет не разделитель */
            if(isspecsymbol(c) == 0)
            {
                skipjob(0);
                fprintf(stderr, "error: event not found, incorrect input\n");
                return;
            }
        }
        /* если само ЧИСЛО некорректное */
        if( (thist >= hist_size) || (thist == 0) )
        {
            mungetchar(cash);
            /* все равно добавляем символ */
            putinhist(c);
            skipjob(0);
            fprintf(stderr, "error: event not found, incorrect number\n");
            return;
        }
        cash_c = c;
        mungetchar(cash);
        hist = thist;
        hist_rec = 0;
        c = mgetchar();
        return;
    }
    
    /* если #comments... */
    if(c == '#')
    {
        /* считываем до конца вводимой команды */
        while ((c != '\n') && (c != EOF))
        {
            /* mungetchar(1); */
            c = mgetchar();
        }
        return;
    }
    
    /* если < inp.txt */
    if(c == '<')
    {
        if( (condition == IN_ARGS) || (condition == IN_OUT) )
        {
            /* переключили на обработку чтения */
            condition = READ;
            c = mgetchar();
            return;
        }
        /* если подставлено не как аргумент к вызываемой программе */
        else
        {
            skipjob(0);
            fprintf(stderr, "error: unexpected '<'\n");
            return;
        }
        
    }
    
    /* если  > out.txt */
    if(c == '>')
    {
        if( (condition == IN_ARGS) || (condition == IN_OUT) )
        {
            c = mgetchar();
            /* в зависимости от управляющего символа 2 варианта состояния */
            if(c == '>')
            {
                c = mgetchar();
                condition = APPEND;
            }
            else
            {
                condition = WRITE;
            }
            return;
        }
        /* неправильная подстановка */
        else
        {
            skipjob(0);
            fprintf(stderr, "error: unexpected '>' (only after name of program | 'prog < inp > out') \n");
            return;
        }
    }

    if(c == '|')
    {
	c = mgetchar();
	if (c == '\n')
	{
		skipjob(0);
		fprintf(stderr, "error: empty conveyor\n");
		return;
	}
    }
    
    /* если ... & */
    if(c == '&')
    {
        /* переключаемся в фоновый режим и обозначаем окончание работы */
        if( (condition == IN_ARGS) || (condition == IN_OUT))
        {
            c = mgetchar();
            condition = END_JOB;
            curr_job.background = 1;
            return;
        }
        /* если вставлено в другом блоке */
        else
        {
            skipjob(0);
            fprintf(stderr, "error: unexpected '&' \n");
            return;
        }
    }
    
    /* если prog1|prog1 */
    if(c == '|')
    {
        if( (condition != IN_PROG) && (condition != READ) && (condition != WRITE) && (condition != APPEND) && (condition != END_JOB) )
        {
            c = mgetchar();
            /* обозначаем ввод имени программы и режима конвеера */
            conveyor = 1;
            condition = IN_PROG;
            return;
        }
        /* пропускаем неправильную часть */
        else
        {
            skipjob(0);
            /* если работа|ее отдельная часть уже кончилась */
            if(condition == END_JOB)
            {
                fprintf(stderr,"error: unexpected '|' or other special symbols \n");
            }
            /* если на этапе ввода программы */
            if(condition == IN_PROG)
            {
                fprintf(stderr,"error: unexpected '|' before program \n");
            }
            /* если после перенаправления ввода/вывода */
            else
            {
                fprintf(stderr,"error: expected file name after '>' or '<' ('<<') \n");
            }
            return;
        }
        
    }
    
    /* ';' => одна программа закончилась */
    if(c == ';')
    {
        /* те состояния, когда ввод ';' некорректен */
        if( (condition != IN_PROG) && (condition != READ) && (condition != WRITE) && (condition != APPEND) )
        {
            c = mgetchar();
            /* для случая ввода ; как последнего символа "...;...;" */
            if(c == '\n')
            {
                mungetchar(1);
            }
            /* сброс параметра конвеера */
            if (conveyor) conveyor = 0;
            /* ожидаем начала ввода новой программы */
            condition = IN_PROG;
            return;
        }
        else
        {
            error = 1;
            /* пустая программа */
            if(condition == IN_PROG)
            {
                fprintf(stderr, "error: entered an empty program (unexpected ';')\n");
            }
            /* после перенаправления ввода/вывода */
            else
            {
                fprintf(stderr, "error: file name expected after '>' or '<' ('<<') \n");
            }
            c = mgetchar();
            if(c == '\n')
            {
                mungetchar(1);
            }
            /* ожидаем начала ввода новой программы */
            condition = IN_PROG;
            return;
        }
    }
    
    /* \n - юзером полностью введена через Enter одна из работ */
    if(c == '\n')
    {
        /* сам символ перехода не заносим */
        mungetchar(1);
        
        if( (condition != READ) && (condition != WRITE) && (condition != APPEND) )
        {
            condition = IN_PROG;
            if (conveyor) conveyor = 0;
            return;
        }
        /* если оборвалось перед вводом имени файла input_file/output_file */
        else
        {
            error = 1;
            fprintf(stderr, "error: file name expected after '>' or '<' ('<<') \n");
            condition = IN_PROG;
            return;
        }
        
    }
    
    /* EOF - окончание ввода, т.е. работы терминала */
    if (c == EOF)
    {
        if( (condition != READ) && (condition != WRITE) && (condition != APPEND) )
        {
            /* обозначаем конец работы шелла */
            condition = END;
            return;
        }
        /* если нажали CTRL+D сразу после ввода '>' или '<' ('<<')  */
        else
        {
            error = 1;
            fprintf(stderr, "error: file name expected after '>' or '<' ('<<') \n");
            condition = END;
            return;
        }
        
    }
    /* выделяем место на символ, по умолчанию '\0' */
    token = (char*) malloc(sizeof(char));
    if(token == NULL)
    {
        printf("\x1b[41mERROR_MALLOC:\x1b[0m");
        mem_error();
        return;
    }
    token[0] = '\0';
    
    while(!isspecsymbol(c))
    {
        /* экранирование '...' */
        if(c == '\'')
        {
            c = mgetchar();
            buf = (char*) malloc(sizeof(char));
            if(buf == NULL)
            {
                printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                mem_error();
                return;
            }
            buf[0] = '\0';

            /* цикл до конца блока '...' или окончания ввода */
            while( (c != '\'') && (c != EOF) )
            {
                /* экранирование обратным слэшом */
                if(c == '\\')
                {
                    c = mgetchar();
                    /* выход на данном этапе некорректен */
                    if(c == EOF)
                    {
                        free(token);
                        free(buf);
                        fprintf(stderr, "error: unexpected EOF in quoting after back-slash \n");
                        error = 1;
                        condition = END;
                        return;
                    }
                    /* учитываем перенос строки после слэша*/
                    if(c == '\n')
                    {
                        c = mgetchar();
                        /* продолжаем считывать новую строку как часть работы */
                        continue;
                    }
                }
                /* выделяем место для очередного символа в кэше */
                cash_str = (char*) realloc(buf, (strlen(buf) + 2) * sizeof(char));
                if(cash_str == NULL)
                {
                    printf("\x1b[41mERROR_REALLOC:\x1b[0m");
                    free(buf);
                    mem_error();
                    return;
                }

                /* пишем последний введенный символ и \0 */
                buf = cash_str;
                buf[strlen(buf) + 1] = '\0';
                buf[strlen(buf)] = c;
                c = mgetchar();
            }
            
            /* если нажато CTRL+D во время ввода блока '...'  */
            if(c == EOF)
            {
                free(token);
                free(buf);
                fprintf(stderr, "error: unexpected EOF in quoting-block '...' \n");
                error = 1;
                condition = END;
                return;
            }
            c = mgetchar();
        }
        else 
        /* экранирование "..." */
        if(c == '\"')
        {
            c = mgetchar();
            buf = (char*) malloc(sizeof(char));
            if(buf == NULL)
            {
                printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                mem_error();
                return;
            }
            buf[0] = '\0';
            /* ан-но цикл до конца блока "..." или окончания ввода */            
            while( (c != '\"') && (c != EOF) )
            {
                /* в случае "..." переменные окружения считываются */
                if(c == '$')
                {
                    /* переход в функцию получения перменной, с параметром для двойных кавычек  */
                    buf_buf = mgetenv(DQ);
                    if(buf_buf == NULL)
                    {
                        free(buf);
                        return;
                    }                    
                }
                /* в остальных случаях ан-но, но с еще одним буфером buf_buf */
                else
                {
                    /* экранирование обратным слэшом */
                    if(c == '\\')
                    {
                        c = mgetchar();
                        if(c == EOF)
                        {
                            error = 1;
                            free(token);
                            free(buf);
                            fprintf(stderr, "\nerror: unexpected EOF in quoting adter back-slash\n");
                            condition = END;
                            return;
                        }
                        /* переход на новую строку ввода после \ */
                        if(c == '\n')
                        {
                            c = mgetchar();
                            continue;
                        }
                    }
                    /* пишем в buf_buf только введенный символ и \0 */
                    cash_str = (char*) malloc(2 * sizeof(char));
                    if(cash_str == NULL)
                    {
                        printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                        free(buf);
                        mem_error();
                        return;
                    }
                    buf_buf = cash_str;
                    buf_buf[0] = c;
                    buf_buf[1] = '\0';
                    c = mgetchar();
                }
 
                /* присоединяем через буфер buf_buf к buf */               
                cash_str = (char*) realloc(buf, (strlen(buf) + strlen(buf_buf) + 1) * sizeof(char));
                if(cash_str == NULL)
                {
                    printf("\x1b[41mERROR_REALLOC:\x1b[0m");
                    free(buf);
                    free(buf_buf);
                    mem_error();
                    return;
                }
                buf = cash_str;
                strcat(buf, buf_buf);
                free(buf_buf);
            }
            /* ан-но считаем некорректным выход до конца блока "..."*/           
            if(c == EOF)
            {
                error = 1;
                free(token);
                free(buf);
                fprintf(stderr, "\nerror: unexpected EOF in quoting-block \"...\" \n");
                condition = END;
                return;
            }
            
            c = mgetchar();
        }
        /* рассматриваем упарвляющие символы, вне "..." или '...' */
        else
        {
            /* замена ${NAME} значением переменной */
            if(c == '$')
            {
                /* вызов по умолчанию */
                buf = mgetenv(0);
                if(buf == NULL)
                {
                    return;
                }
            }
            else
            {
                /* ан-но с пред. частями - экранирование \ */
                if(c == '\\')
                {
                    c = mgetchar();
                    if(c == EOF)
                    {
                        error = 1;
                        fprintf(stderr, "\nerror: exit after back-slash \n");
                        free(token);
                        token = NULL;
                        condition = END;
                        return;
                    }
                    if(c == '\n')
                    {
                        c = mgetchar();
                        continue;
                    }
                    
                }
                /* оставшиеся варианты - просто символы  */
                buf = (char*) malloc(2 * sizeof(char));
                if(buf == NULL)
                {
                    printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                    mem_error();
                    return;
                }
                buf[0] = c;
                buf[1] = '\0';
                c = mgetchar();
            }
        }
        cash_str = (char*) realloc(token, (strlen(token) + strlen(buf) + 1) * sizeof(char));
        if(cash_str == NULL)
        {
            printf("\x1b[41mERROR_REALLOC:\x1b[0m");
            free(buf);
            mem_error();
            return;
        }
        /* в token теперь лежит обработанная часть программы */
        token = cash_str;  
        strcat(token, buf);
        /* printf("cur_token = %s \n", token); */
        free(buf);
    }
    /*== анализ состояния работы ==*/
    /* когда дошли до конца одной программы|начинаем ввод аргументов */
    if(condition == IN_PROG)
    {
        /* расширяем структуру programs */
        cash_programs = (struct program*) realloc(curr_job.programs, (curr_job.number_of_programs + 1) * sizeof(struct program));
        if(cash_programs == NULL)
        {
            printf("\x1b[41mERROR_REALLOC:\x1b[0m");
            mem_error();
            return;
        }
        /* учитываем изменения */
        curr_job.number_of_programs++;
        curr_job.programs = cash_programs;
        /* считываем аргументы */     
        cash_arguments = (char**) malloc(2 * sizeof(char*));
        if(cash_arguments == NULL)
        {
            printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return;
        }
        /* (CURR_PROG = curr_job.programs[curr_job.number_of_programs - 1) */
        CURR_PROG.arguments = cash_arguments;
        /* записываем название вызываемой программы */
        CURR_PROG.arguments[0] = token;
        CURR_PROG.name = token;
        /* (последний аргумент всегда нулевой) */
        CURR_PROG.arguments[1] = NULL;
        CURR_PROG.number_of_arguments = 2;
        CURR_PROG.input_file = NULL;
        CURR_PROG.output_file = NULL;
        CURR_PROG.output_type = 0;
        /* меняем состояние на считываение аргументов вызываемйо программы */
        condition = IN_ARGS;
        token = NULL;
        return;
    }
    /* если на этапе ввода аргументов вызываемой программы */
    if(condition == IN_ARGS)
    {
        /* расширили массив строк под аргументы */
        cash_arguments = (char**) realloc(CURR_PROG.arguments, (CURR_PROG.number_of_arguments + 1) * sizeof(char*));
        if(cash_arguments == NULL)
        {
            printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return;
        }
        /* учитываем изменения в структуре текущей программы */
        CURR_PROG.number_of_arguments++;
        CURR_PROG.arguments = cash_arguments;
        (CURR_PROG.arguments)[CURR_PROG.number_of_arguments - 2] = token;
        (CURR_PROG.arguments)[CURR_PROG.number_of_arguments - 1] = NULL;
        token = NULL;
        return;
    }
    /* если был ранее уже ввели output_file/input_file */
    if(condition == IN_OUT)
    {
        free(token);
        token = NULL;
        skipjob(0);
        fprintf(stderr, "error: unexpected argument after '>' or '<' ('<<') \n");
        return;
    }
    /* изменение input_file */
    if(condition == READ)
    {
        /* printf("перезапись input_file\n"); */
        free(CURR_PROG.input_file);
        CURR_PROG.input_file = token;
        token = NULL;
        condition = IN_OUT;
        return;
    }
    /* изменение output_file  и output_type */
    if(condition == WRITE)
    {
        /* printf("перезапись output_file\n"); */
        free(CURR_PROG.output_file);
        CURR_PROG.output_file = token;
        token = NULL;
        CURR_PROG.output_type = 1;
        condition = IN_OUT;
        return;
    }
    if(condition == APPEND)
    {
        /* printf("перезапись output_file\n"); */
        free(CURR_PROG.output_file);
        CURR_PROG.output_file = token;
        token = NULL;
        CURR_PROG.output_type = 2;
        condition = IN_OUT;
        return;
    }
    /* если юзер ввел что-то после переключения в фоновый режим */
    if(condition == END_JOB)
    {
        skipjob(0);
        fprintf(stderr, "error: nothing is expected after '&'\n");
        return;
    }
}

/*  -  -  -  -  -  -  -  -  -*/
/*     ОБРАБОТЧИК КОМАНД     */
/*  -  -  -  -  -  -  -  -  -*/
void job_input()
{
    clear_job();
    /* (начальные условия) */
    conveyor = 0;
    curr_job.background = 0;
    curr_job.number_of_programs = 0;
    curr_job.programs = NULL;
    error = 0;
    /* если дошли до конца строки (по умолчанию c='\n') */
    if(c == '\n')
    {        
        /* выделение нового элемента истории: на строку и на символ */
        if( (hist_size == 0) || (history[hist_size - 1][0] != '\0') )
        {
            history = (char **) realloc(history, (++hist_size) * sizeof(char*));
            history[hist_size - 1] = (char *) malloc(sizeof(char));
            history[hist_size - 1][0] = '\0';
            hist_curr = 0;
        }
        /* готовы считывать новую */
	    printf("\x1b[1;32m%s$ \x1b[0m", getenv("USER"));
        c = mgetchar();
    }
    /* считывваем до конца текущей программы|перенаправления конвеером */
    do
    {
        getsymbol();
    }
    while( (condition != END) && ( (condition != IN_PROG) || (conveyor != 0) ) );
    /* если что-то пошло не так-очищаем память работы */
    if(error == 1)
    {
        clear_job();
        curr_job.background = 0;
        curr_job.number_of_programs = 0;
        curr_job.programs = NULL;
    }
    return;
}

/* функция проверки на управляющий символ/разделитель(экранирование не в счет) */
int isspecsymbol(int c)
{
    if   ((c == ';' )
       || (c == '|' )
       || (c == '&' )
       || (c == '\n')
       || (c == ' ' )
       || (c == '\t')
       || (c == EOF )
       || (c == '>' )
       || (c == '<'))
       {
           return 1;
       }
       else
       {
           return 0;
       }
}

/* [для тестирования]
void printjob()
{
    int i, j;
    if(curr_job.number_of_programs == 0)
        return;
    printf("Job:\nBackground = %d\n", curr_job.background);
    for(i = 0; i < curr_job.number_of_programs; i++)
    {
        printf("\tProgram #%d: %s\n", i, (curr_job.programs[i]).name);
        for(j = 0; j < (curr_job.programs[i]).number_of_arguments; j++)
        {
            printf("\t\tArguement #%d: %s\n", j, (curr_job.programs[i]).arguments[j]);
        }
        if((curr_job.programs[i]).input_file != NULL)
            printf("\tFrom \"%s\"\n", (curr_job.programs[i]).input_file);
        else
            printf("\tFrom stdin\n");
        if((curr_job.programs[i]).output_file != NULL)
            printf("\tTo \"%s\" (in mode %d)\n\n", (curr_job.programs[i]).output_file, (curr_job.programs[i]).output_type);
        else
            printf("\tTo stdout\n\n");
    }
}
*/

/* очищает память текущей работы */
void clear_job()
{
    int i, j;
    /* цикл по всем программам работы */
    for(i = 0; i < curr_job.number_of_programs; i++)
    {
        /* цикл по всем аргументам программы */
        for(j = 1; j < (curr_job.programs[i]).number_of_arguments; j++)
        {
            free((curr_job.programs[i]).arguments[j]);
        }
        /* очистка остальных строк */
        free((curr_job.programs[i]).arguments);
        free((curr_job.programs[i]).name);
        free((curr_job.programs[i]).input_file);
        free((curr_job.programs[i]).output_file);
    }
    free(curr_job.programs);
}

/* обработка ошибок */
void mem_error()
{
    free(token);
    error = 1;
    /* только для критических ошибок => выходим из программы */
    condition = END;
}

/* функция возобновляет обработку строки в случае некритичных ошибок */
void skipjob(int reg)
{
    /* т.к. функция вызывается только в ошибочных ситуациях */
    error = 1;
    condition = END_JOB;

    /* разные варианты обработки : одинарные и двойные кавычки */    
    if(reg == SQ) goto S;
    if(reg == DQ) goto D;

    /* движемся до конца команды */    
    while( (c != '\n') && (c != EOF) && (c != ';'))
    {
        /* при экранировании считываем сразу 2 */
        if(c == '\\')
        {
            c = mgetchar();
            if(c != EOF)
            {
                c = mgetchar();
            }
            continue;
        }
 
        /* экранирование блока в одинарных кавычках */       
        if(c == '\'')
        {
            c = mgetchar();
S:  	    while( (c != '\'') && (c != EOF) )
            {
                c = mgetchar();
                if(c == '\\')
                {
                    c = mgetchar();
                    if(c != EOF)
                    {
                        c = mgetchar();
                    }
                }
            }
            if(c == '\'')
            {
                c = mgetchar();
            }
            continue;
        }

        /* экранирование блока в двойных кавычках */       
        if(c == '\"')
        {
            c = mgetchar();
D:  	    while( (c != '\"') && (c != EOF) )
            {
                c = mgetchar();
                if(c == '\\')
                {
                    c = mgetchar();
                    if(c != EOF)
                    {
                        c = mgetchar();
                    }
                }
            }
            if(c == '\"')
            {
                c = mgetchar();
            }
            continue;
        }
 
        /* если встечаем пустые места */       
        if( (c == ' ') || (c == '\t') )
        {
            c = mgetchar();
            /* если встретили #comments... - считываем до конца строки, но (не) заносим в историю */
            if(c == '#')
            {
                while( (c != '\n') && (c != EOF) )
                {
                    /* mungetchar(1); */
                    c = mgetchar();
                }
            }
            continue;
        }
        c = mgetchar();
    }
}

/*== определяем системные параметры ==*/
void starting()
{
    /* PID шелла */
    cash_str = malloc((SIZEOFPID + 1) * sizeof(char));
    if(cash_str == NULL)
    {
		printf("\x1b[41mERROR_MALLOC:\x1b[0m");
        condition = END;
        return;
    }
    snprintf(cash_str, SIZEOFPID, "%d", getpid());
    /* сразу поставили переменную окружения ${PID} */
    setenv("PID", cash_str, 0);
    free(cash_str);

    /* ${UID} пользователя */
    cash_str = malloc((SIZEOFUID + 1) * sizeof(char));
    if(cash_str == NULL)
    {
		printf("\x1b[41mERROR_MALLOC:\x1b[0m");
        condition = END;
        return;
    }
    snprintf(cash_str, SIZEOFUID, "%d", getuid());
    setenv("UID", cash_str, 0);
    free(cash_str);

    /* переменная ${PWD} */
    cash_str = getcwd(NULL, 0);
    if(cash_str == NULL)
    {
		printf("\x1b[41mERROR_CWD:\x1b[0m");
        condition = END;
        return;
    }
    setenv("PWD", cash_str, 1);
    free(cash_str);

    /* игнорируем попытку приостановки для данного процесса */
    signal(SIGTTOU, SIG_IGN);
}
/*  -  -  -  -  -  -  -  -  -  -  -  -  */
/*    СЧИТЫВАТЕЛЬ ПЕРЕМЕННОЙ ПОСЛЕ $    */
/*  -  -  -  -  -  -  -  -  -  -  -  -  */
char *mgetenv(int reg)
{
    char *var = NULL;
    char *name = NULL;
    unsigned int num = 0;
    int i = 1;
    /* по этому считанному символу определим, как считывать */
    c = mgetchar();
    /* значение статуса последнего завершившегося процесса */
    if(c == '?')
    {
        c = mgetchar();
        /* (last_condition устанавливается в procshell.c как статус завершившегося в переднем плане) */
        for(num = last_condition, i = 1; (num /= 10) > 0; i++);
        /* теперь i - кол-во цифр в этом числе */
        var = (char*) malloc((i + 1) * sizeof(char));
        if(var == NULL)
        {
		    printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return NULL;
        }
        /* перевод числа num в строку var по цифрам */
        for(num = last_condition, var[i--] = '\0'; i >= 0; num /= 10, i--)
        {
            var[i] = num % 10 + '0';
        }
        return var;
    }
    /* кол-во параметров шелла */
    if(c == '#')
    {
        c = mgetchar();
        /* ан-но через деление нацело считаем i */
        for(num = margc, i = 1; (num /= 10) > 0; i++);
        var = (char*) malloc((i + 1) * sizeof(char));
        if(var == NULL)
        {
		    printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return NULL;
        }
        /* перевод числа num в строку var по цифрам */
        for(num = margc, var[i--] = '\0'; i >= 0; num /= 10, i--)
        {
            var[i] = num % 10 + '0';
        }
        return var;
    }
    /* $ЧИСЛО - берем соотв. параметр шелла */
    if( (c >= '0') && (c <= '9') )
    {
        /* движемся по цифрам-символам и переводим в число */
        while( (c >= '0') && (c <= '9') )
        {
            num *= 10;
            num += c - '0';
            c = mgetchar();
        }
        /* если ЧИСЛО слишком большое */
        if(num >= margc)
        {
            var = (char*) malloc(sizeof(char));
            if(var == NULL)
            {
		        printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                mem_error();
                return NULL;
            }
            var[0] = '\0';
            /* возвращаем пустое (как в bash) */
            return var;
        }
        /* копируем в var margv[ЧИСЛО] */
        var = (char*) malloc((strlen(margv[num]) + 1) * sizeof(char));
        if(var == NULL)
        {
            printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return NULL;
        }
        strcpy(var, margv[num]);
        return var;
    }
    /* если ${NAME} */
    if(c == '{')
    {
        c = mgetchar();
        name = (char*) malloc(sizeof(char));
        if (name == NULL)
        {
            printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return NULL;
        }
        name[0] = '\0';
        /* движемся по буквам, фифрам и подчеркиваниям */      
        while( (isalpha(c) != 0) || (isdigit(c) != 0) || (c == '_') )
        {
            /* выделяем место на еще один символ в буфере */
            cash_str = (char*) realloc(name, (strlen(name) + 2) * sizeof(char));
            if(cash_str == NULL)
            {
                printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                free(name);
                mem_error();
                return NULL;
            }
            name = cash_str;
            name[strlen(name) + 1] = '\0';
            name[strlen(name)] = c;
            c = mgetchar();
        }        
        cash_str = NULL;
        /* если после NAME нет закрывающей скобки */       
        if(c != '}') /* || (isdigit(name[0]) != 0) ) */ 
        {
            /* (reg - параметр функции для учета кавычек) */
            skipjob(reg);
            /* если вышли до ввода } */
            if(c == EOF)
            {
                putchar('\n');
            }
            fprintf(stderr, "error: incorrect ${%s} input \n", name);
            free(name);
            free(token);
            return NULL;
        }
        
        c = mgetchar();
        /* непосредственно получение переменной окружения */
        cash_str = getenv(name);
        free(name);        
        if(cash_str == NULL)
        {
            var = (char*) malloc(sizeof(char));
            if(var == NULL)
            {
                printf("\x1b[41mERROR_MALLOC:\x1b[0m");
                mem_error();
                return NULL;
            }
            var[0] = '\0';
            return var;
        }
        /* в var копируем искомый результат */       
        var = malloc((strlen(cash_str) + 1) * sizeof(char));
        if(var == NULL)
        {
            printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return NULL;
        }
        strcpy(var, cash_str);
        cash_str = NULL;
        return var;
    }
    /* если ни один из вариантов запроса переменной не обнаружили */
    else
    {
        var = (char*) malloc(2 * sizeof(char));
        if (var == NULL)
        {
            printf("\x1b[41mERROR_MALLOC:\x1b[0m");
            mem_error();
            return NULL;
        }
        /* возвращаем просто $ */
        var[0] = '$';
        var[1] = '\0';
        return var;
    }
}

/* встроенная функция cd */
void mcd()
{
    /* отдельно рассмотрим команду cd => домой */
    if(curr_job.programs[0].number_of_arguments == 2)
    {
        if(chdir(getenv("HOME")) < 0)
        {
            perror("ERROR_CHDIR_HOME");
        }
        setenv("PWD", getenv("HOME"), 1);
        printf("new directory:\n%s \n", getenv("HOME"));
        return;
    }
    /* если не был введен параметр */
    if(curr_job.programs[0].number_of_arguments > 3)
    {
        printf("cd: invalid input of arguments\n");
    }
    else 
    if(chdir(curr_job.programs[0].arguments[1]) < 0)
    {
        perror("ERROR_CHDIR");
    }
    else
    {
        cash_str = getcwd(NULL, 0);
        if(cash_str == NULL)
        {
		    printf("\x1b[41mERROR_GETCWD:\x1b[0m");
            condition = END;
            return;
        }
        printf("new directory:\n%s \n", cash_str);
        /* сразу меняем переменную ${PWD} */
        setenv("PWD", cash_str, 1);
        free(cash_str);
    }
}

/* считываение символа с потока ввода|искомой команды и добавление его в историю */
int mgetchar()
{
    int ch;
    /* если не поднят флаг вызова из истории */
    if(hist == 0)
    {
        ch = getchar();
    }
    else
    {
        /* берем след. символ из строки заданной параметром !ЧИСЛО */
        ch = history[hist - 1][hist_rec++];
        /* искомая команда считана */
        if(ch == '\0')
        {
            hist = 0;
            ch = cash_c;
        }     
    }	
    /* выделили еще места в истории */
    cash_str = realloc(history[hist_size - 1], (hist_curr + 2) * sizeof(char));
    if(cash_str == NULL)
    {
        printf("\x1b[41mERROR_REALLOC:\x1b[0m");
        mem_error();
        return EOF;
    }
    /* на случай, если изменилось расположение выделенного блока */
    history[hist_size - 1] = cash_str;
    if(ch != EOF)
    {
        history[hist_size - 1][hist_curr++] = (char) ch;
    }
    else
    {
        history[hist_size - 1][hist_curr++] = '\n';	
    }
    history[hist_size - 1][hist_curr] = '\0';
    return ch;
}

/* функция приписывает символ ch в конец текущей строки истории */
void putinhist(int ch)
{
    cash_str = realloc(history[hist_size - 1], (hist_curr + 2) * sizeof(char));
    if(cash_str == NULL)
    {
        printf("\x1b[41mERROR_REALLOC:\x1b[0m");
        mem_error();
        return;
    }
    history[hist_size - 1] = cash_str;
    if(ch != EOF)
    {
        history[hist_size - 1][hist_curr++] = (char) ch;
    }
    /* обозначаем в истории конец ввода */
    else
    {
        history[hist_size - 1][hist_curr++] = '\n';	
    }
    history[hist_size - 1][hist_curr] = '\0';
}

/* функция "отменяет" считывание символа */
void mungetchar(int step)
{
    if((hist_curr -= step) < 0)
    {
        return;
    }
    history[hist_size - 1][hist_curr] = '\0';
    history[hist_size - 1] = realloc(history[hist_size - 1], (hist_curr + 1) * sizeof(char));
}

/* функция выводит историю построчно */
void historyprint()
{
    int i, lim;
    
    lim = hist_size;
    /* учитываем случай, что history еще не последняя команда в работе */
    if(c != '\n')
    {
        lim--;
    }
    for(i = 0; i < lim; i++)
    {
        printf("[%d] - %s\n", i + 1, history[i]);
    }
}

/* очищаем все строки истории */
void clear_hisory()
{
    int i;
    for(i = 0; i < hist_size; i++)
    {
        free(history[i]);
    }
    free(history);
    hist_size = 0;
}
