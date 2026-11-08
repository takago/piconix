/* PicoNIX educational C subset compiler: C source -> native as source.
 * Deliberately small: user functions (up to four int arguments),
 * stack-based locals, if/while, read/write, and Thumb-1 code generation.
 * No libc or preprocessor. Single-level int/char pointers are supported;
 * pointer bounds, pointer returns and pointer-pointer subtraction are not.
 */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

#define SOURCE_MAX 1024
#define ASM_MAX 4096
#define ASM_FILE_MAX 2048 /* RAM file and as source limit */
static char source[SOURCE_MAX+1], output[ASM_MAX+1];
static int at, used, labels, failed, nvars, output_overflow;
static int function_end, function_void;
static char functions[16][20], calls[32][20];
static int function_args[16], call_args[32], param_kinds[16][4], call_kinds[32][4], nfunctions, ncalls, have_main;
static char vars[16][20];
static int offsets[16], lengths[16], types[16], arrays[16], ptrs[16], frame_used; /* 1=char, 4=int */

static int letter(int c) {return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';}
/* 入力文字が十進数字であるか判定する． */
static int digit(int c) {return c>='0'&&c<='9';}
/* 空白とC形式のコメントを読み飛ばす． */
static void ws(void) {
    for (;;) {
        while(source[at]==' '||source[at]=='\t'||source[at]=='\r'||source[at]=='\n')at++;
        if(source[at]=='/'&&source[at+1]=='/') {while(source[at]&&source[at]!='\n')at++;continue;}
        if(source[at]=='/'&&source[at+1]=='*') {
            at+=2;while(source[at]&&!(source[at]=='*'&&source[at+1]=='/'))at++;
            if(!source[at])failed=1;else at+=2;
            continue;
        }
        break;
    }
}
/* 指定した予約語を単語境界も確認して読み取る． */
static int word(const char *w) {
    int n=(int)strlen(w);ws();
    if(strncmp(source+at,w,(size_t)n)||letter(source[at+n])||digit(source[at+n]))return 0;
    at+=n;return 1;
}
/* 次の1文字が指定した記号なら消費する． */
static int token(char c) {ws();if(source[at]==c){at++;return 1;}return 0;}
/* 指定した記号を要求し，見つからなければエラーにする． */
static void need(char c) {if(!token(c))failed=1;}
/* 符号付き十進整数を範囲検査しながら読み取る． */
static int integer(void) {
    uint32_t v=0;int sign=1;ws();if(source[at]=='-'){sign=-1;at++;}
    if(!digit(source[at])){failed=1;return 0;}
    while(digit(source[at])) {
        unsigned d=(unsigned)(source[at++]-'0');
        if(v>214748364u || (v==214748364u && d>(sign<0?8u:7u))) {
            failed=1;return 0;
        }
        v=v*10u+d;
    }
    if(sign<0) {if(v==2147483648u)return (int)(-2147483647-1);return -(int)v;}
    return (int)v;
}
/* Keep labels/directives at column zero, indent Thumb instructions by two
 * spaces.  emit() also accepts fragments of a line (notably .ascii data).
 * The state therefore follows newlines across separate emit() calls. */
static int line_start=1;
/* 生成中のThumbアセンブリを出力領域へ追加する． */
static void emit(const char *s) {
    while(*s) {
        if(line_start && *s!='\n') {
            /* All generated labels contain ':' before the end of the line. */
            int label=0;
            if(*s!='.') {
                const char *p=s;
                while(*p && *p!='\n') {
                    if(*p==':'){label=1;break;}
                    p++;
                }
            }
            if(*s!='.' && !label) {
                if(used+2>=ASM_MAX){failed=1;output_overflow=1;return;}
                output[used++]=' ';
                output[used++]=' ';
            }
            line_start=0;
        }
        if(used+1>=ASM_MAX){failed=1;output_overflow=1;return;}
        output[used++]=*s;
        if(*s=='\n')line_start=1;
        s++;
    }
    output[used]=0;
}
/* 8ビット即値を使う命令を生成する． */
static void imm(const char *instruction,int v) {
    char line[48];if(v<0||v>255){failed=1;return;}
    snprintf(line,sizeof line,"%s #%d\n",instruction,v);emit(line);
}
/* レジスタ番号と8ビット即値を指定して命令を生成する． */
static void regimm(const char *op,int reg,int value) {
    char line[48]; if(value<0||value>255){failed=1;return;}
    snprintf(line,sizeof line,"%s r%d, #%d\n",op,reg,value);emit(line);
}
/* Materialize signed 32-bit constants with Thumb-1 movs/adds only.
 * For wide values, build the bit pattern by doubling r0 and adding 1 bits.
 * The assembler deliberately supports neither literal pools nor wide movs. */
static void constant(int value) {
    uint32_t bits=(uint32_t)value;
    if(bits<=255u){regimm("movs",0,(int)bits);return;}
    int bit=31;
    while(bit>0 && !(bits&(1u<<bit)))bit--;
    regimm("movs",0,1);
    for(bit--;bit>=0;bit--) {
        emit("adds r0, r0\n");
        if(bits&(1u<<bit))emit("adds r0, #1\n");
    }
}
/* レジスタ間転送命令を生成する． */
static void regmove(int dst,int src) {
    char line[32];snprintf(line,sizeof line,"mov r%d, r%d\n",dst,src);emit(line);
}
/* 指定ラベルへの分岐命令を生成する． */
static void branch(const char *op,int id) {
    char line[32];snprintf(line,sizeof line,"%s L%d\n",op,id);emit(line);
}
/* 分岐先ラベルを出力する． */
static void mark(int id) {char line[32];snprintf(line,sizeof line,"L%d:\n",id);emit(line);}
/* C識別子を読み取って名前を保存する． */
static int identifier(char *out) {
    int n=0;ws();if(!letter(source[at])){failed=1;return 0;}
    while(letter(source[at])||digit(source[at])) {
        if(n>=19){failed=1;return 0;}out[n++]=source[at++];
    }out[n]=0;return 1;
}
/* 文字リテラルと一部のエスケープを読み取る． */
static int character(void) {
    int c; ws(); if(source[at++]!='\''){failed=1;return 0;}
    c=(unsigned char)source[at++];
    if(c=='\\'){c=source[at++];if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';else if(c=='0')c=0;}
    if(source[at++]!='\'')failed=1;
    return c;
}
/* 変数名からローカル変数表の位置を求める． */
static int variable(const char *name) {
    for(int i=0;i<nvars;i++)if(!strcmp(name,vars[i]))return i;
    failed=1;return 0;
}
/* r4 is a callee-saved frame pointer: stack temporaries must not shift
 * references to locals and parameters within the fixed 256-byte frame. */
static void local_access(const char *op,int reg,int index) {
    char line[64];
    int offset=offsets[index];
    if(offset<=124) {
        snprintf(line,sizeof line,"%s r%d, [r4, #%d]\n",op,reg,offset);
        emit(line);
    } else {
        /* r4 is the stable frame pointer; r2 is a temporary address. */
        emit("mov r2, r4\n");
        snprintf(line,sizeof line,"adds r2, #%d\n",offset);emit(line);
        snprintf(line,sizeof line,"%s r%d, [r2, #0]\n",op,reg);emit(line);
    }
}
/* Recursive-descent expression parser. The result is always in r0.
 * push/pop preserve intermediate values across recursive subexpressions.
 * Arithmetic is 32-bit; division and remainder currently require nonnegative
 * operands. Division by zero is rejected at runtime by returning zero.
 */
static int value_ptr; /* 0=integer, 1=char*, 4=int* */
static void expression(int dst);
static void address(int index);
/* Evaluate arguments left-to-right. Preserve fd and buffer address while
 * parsing the count expression; SVC expects call,r1=fd,r2=buffer,r3=count.
 * The parser uses r0/r1/r2 internally, so restore registers last. */
static void syscall_buffer(int call) {
    need('(');
    expression(0);
    emit("push {r0}\n");
    need(',');expression(0);
    if(!value_ptr)failed=1;
    emit("push {r0}\n");
    need(',');expression(0);need(')');
    emit("mov r3, r0\npop {r2}\npop {r1}\n");
    regimm("movs",0,call);
    emit("svc #0\n");value_ptr=0;
}

/* ローカル変数のフレーム内アドレスを生成する． */
static void address(int index) {
    char line[48];
    emit("mov r0, r4\n");
    snprintf(line,sizeof line,"adds r0, #%d\n",offsets[index]);emit(line);
}
/* Calculate array-element address, for both real arrays and pointers. */
static void array_address(int index) {
    int size=ptrs[index]?ptrs[index]:types[index];
    if(!arrays[index] && !ptrs[index])failed=1;
    if(ptrs[index])local_access("ldr",0,index);else address(index);
    emit("push {r0}\n");need('[');expression(0);need(']');
    if(size==4)emit("adds r0, r0\nadds r0, r0\n");
    emit("pop {r1}\nadds r0, r1\n");
}
/* Increment/decrement an int local. r0 holds old or new expression value.
 * Pointers, arrays and char lvalues are intentionally unsupported. */
static void incdec(int index,int delta,int postfix) {
    if(arrays[index] || ptrs[index] || types[index]!=4) { failed=1; return; }
    local_access("ldr",0,index);
    emit(delta>0 ? "adds r0, #1\n" : "subs r0, #1\n");
    local_access("str",0,index);
    if(postfix) emit(delta>0 ? "subs r0, #1\n" : "adds r0, #1\n");
    value_ptr=0;
}
/* Parse ++ or -- as a single token, never as two unary operators. */
static int step_token(void) {
    ws();
    if(source[at]=='+' && source[at+1]=='+') { at+=2; return 1; }
    if(source[at]=='-' && source[at+1]=='-') { at+=2; return -1; }
    return 0;
}
/* 配列またはポインタ要素を読み出す命令を生成する． */
static void load_element(int index) {
    array_address(index);
    emit((ptrs[index]?ptrs[index]:types[index])==1?"ldrb r0, [r0, #0]\n":"ldr r0, [r0, #0]\n");
}
/* AAPCS: up to four int parameters in r0-r3, result in r0.
 * Keep SP 8-byte aligned at each call site, even for odd argument counts. */
static void function_call(const char *name) {
    char line[48];int count=0;
    need('(');
    /* Count arguments before parsing expressions; preserve the function's
     * frame-relative offsets while each argument is evaluated. */
    int save=at,depth=0;
    ws();
    if(source[at]!=')') {
        count=1;
        for(int i=at;source[i];i++) {
            if(source[i]=='('||source[i]=='[')depth++;
            else if(source[i]==')'||source[i]==']') {
                if(depth==0)break;
                depth--;
            } else if(source[i]==',' && depth==0)count++;
        }
    }
    if(count>4 || ncalls>=32){failed=1;return;}
    strcpy(calls[ncalls],name);call_args[ncalls++]=count;
    at=save;
    for(int i=0;i<count;i++) {
        if(i)need(',');
        expression(0);call_kinds[ncalls-1][i]=value_ptr;
        emit("push {r0}\n");
    }
    need(')');
    for(int i=count-1;i>=0;i--) {
        snprintf(line,sizeof line,"pop {r%d}\n",i);emit(line);
    }
    /* Only adjust SP after evaluating all arguments: local variable
     * offsets are relative to the original function stack frame. */
    if(count&1)emit("sub sp, #4\n");
    snprintf(line,sizeof line,"bl f_%s\n",name);emit(line);
    if(count&1)emit("add sp, #4\n");
}
/* 式の基本要素を解析して評価コードを生成する． */
static void primary(void) {
    char name[20];
    if(token('(')){expression(0);need(')');}
    else if(word("read"))syscall_buffer(8);
    else if(word("sizeof")) {
        need('(');identifier(name);int i=variable(name);need(')');regimm("movs",0,arrays[i]?types[i]*lengths[i]:(ptrs[i]?4:types[i]));value_ptr=0;
    } else {ws();if(source[at]=='\''){regimm("movs",0,character());value_ptr=0;}
        else if(letter(source[at])) {
            identifier(name);ws();
            if(source[at]=='(') {function_call(name);value_ptr=0;return;}
            int i=variable(name);
            int post=step_token();
            if(post) { incdec(i,post,1); return; }
            if(source[at]=='[' || (ws(),source[at]=='[')) {load_element(i);value_ptr=0;}
            else if(arrays[i]){address(i);value_ptr=types[i];}
            else if(types[i]==1 && !ptrs[i]){address(i);emit("ldrb r0, [r0, #0]\n");value_ptr=0;}
            else {local_access("ldr",0,i);value_ptr=ptrs[i];}
        } else {constant(integer());value_ptr=0;}}
}
/* 単項演算子とオペランドを解析する． */
static void unary(void) {
    int step=step_token();
    if(step) { char name[20]; identifier(name); incdec(variable(name),step,0); }
    else if(token('+'))unary();
    else if(token('-')){unary();if(value_ptr)failed=1;emit("movs r1, #0\nsubs r1, r0\nmov r0, r1\n");}
    else if(token('&')) {
        char name[20];identifier(name);int i=variable(name);
        ws();
        if(source[at]=='[')array_address(i);
        else {if(arrays[i])failed=1;address(i);}
        value_ptr=ptrs[i]?4:types[i]; /* address of pointer itself is unsupported */
        if(ptrs[i])failed=1;
    }
    else if(token('*')) {
        unary();
        int size=value_ptr;if(!size)failed=1;
        emit(size==1?"ldrb r0, [r0, #0]\n":"ldr r0, [r0, #0]\n");value_ptr=0;
    }
    else primary();
}
/* 乗除算など乗法優先順位の式を解析する． */
static void multiply(void) {
    unary();
    while(!failed){
        ws();char op=source[at];if(op!='*'&&op!='/'&&op!='%')break;at++;
        if(value_ptr)failed=1;emit("push {r0}\n");unary();if(value_ptr)failed=1;emit("pop {r1}\n");
        if(op=='*')emit("muls r0, r1\n");
        else {
            int loop=labels++,end=labels++,zero=labels++;
            /* r1=dividend, r0=divisor; r2=quotient. */
            emit("movs r2, #0\ncmp r0, #0\n");branch("beq",zero);
            mark(loop);emit("cmp r1, r0\n");branch("bcc",end);
            emit("subs r1, r0\nadds r2, #1\n");branch("b",loop);
            mark(end);emit(op=='/'?"mov r0, r2\n":"mov r0, r1\n");branch("b",zero);mark(zero);
        }
    }
}
/* 加減算など加法優先順位の式を解析する． */
static void additive(void) {
    multiply();
    while(!failed){
        ws();char op=source[at];if(op!='+'&&op!='-')break;at++;
        int left=value_ptr;
        emit("push {r0}\n");multiply();int right=value_ptr;emit("pop {r1}\n");
        if(left && right){failed=1;continue;} /* no pointer+pointer */
        if(left && !right && left==4)emit("adds r0, r0\nadds r0, r0\n");
        if(right && !left && right==4)emit("adds r1, r1\nadds r1, r1\n");
        if(op=='+'){emit("adds r0, r1\n");value_ptr=left?left:right;}
        else {if(right)failed=1;emit("subs r1, r0\nmov r0, r1\n");value_ptr=left;}
    }
}
/* 式全体を解析して結果のコードを生成する． */
static void expression(int dst) {additive();if(dst!=0)regmove(dst,0);}
/* 条件式の評価と分岐を生成する． */
static void condition(int false_label) {
    char op[3]={0};
    expression(0);emit("push {r0}\n");ws();
    if(source[at]=='='&&source[at+1]=='='){op[0]='=';at+=2;}
    else if(source[at]=='!'&&source[at+1]=='='){op[0]='!';at+=2;}
    else if(source[at]=='<'&&source[at+1]=='='){op[0]='<';op[1]='=';at+=2;}
    else if(source[at]=='>'&&source[at+1]=='='){op[0]='>';op[1]='=';at+=2;}
    else if(source[at]=='<'||source[at]=='>'){op[0]=source[at++];}
    else {failed=1;return;}
    expression(0);emit("pop {r1}\ncmp r1, r0\n");
    if(!strcmp(op,"="))branch("bne",false_label);
    else if(!strcmp(op,"!"))branch("beq",false_label);
    else if(!strcmp(op,"<"))branch("bge",false_label);
    else if(!strcmp(op,">="))branch("blt",false_label);
    else if(!strcmp(op,">"))branch("ble",false_label);
    else if(!strcmp(op,"<="))branch("bgt",false_label);
    else failed=1;
}
/* 関数末尾の復帰処理を生成する． */
static void finish(int status) {
    imm("movs r0,",3);imm("movs r1,",status);emit("svc #0\n");
}
static void statement(void);
/* 中括弧で囲まれた文の列を解析する． */
static void block(void) {
    need('{');while(!failed){ws();if(token('}'))return;if(!source[at]){failed=1;return;}statement();}
}
/* 代入や制御構文などの文を解析する． */
static void statement(void) {
    /* Support ++i; and --i; as standalone statements. */
    int step=step_token();
    if(step) { char name[20]; identifier(name); incdec(variable(name),step,0); need(';'); return; }
    int type=word("int")?4:(word("char")?1:0);
    if(type) {
        int ptr=token('*')?type:0;
        char name[20];identifier(name);if(nvars>=16){failed=1;return;}
        for(int i=0;i<nvars;i++)if(!strcmp(name,vars[i]))failed=1;
        if(failed)return;
        int len=1,is_array=0;
        if(token('[')){is_array=1;len=integer();need(']');if(len<1||len>64)failed=1;}
        if(ptr && is_array)failed=1;
        int slot=nvars++;strcpy(vars[slot],name);types[slot]=ptr?4:type;ptrs[slot]=ptr;lengths[slot]=len;arrays[slot]=is_array;
        frame_used=(frame_used+3)&~3;
        offsets[slot]=frame_used;frame_used+=(types[slot]*len+3)&~3;
        if(frame_used>256)failed=1;
        if(token('=')) {
            if(type==1 && is_array && token('"')) {
                int j=0;
                while(source[at] && source[at]!='"' && !failed){
                    int c=(unsigned char)source[at++];
                    if(c=='\\'){c=source[at++];if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';else if(c=='0')c=0;}
                    if(j>=len){failed=1;break;}
                    address(slot);emit("push {r0}\n");
                    regimm("movs",0,j++);emit("pop {r1}\nadds r0, r1\n");
                    emit("mov r1, r0\n");regimm("movs",0,c);emit("strb r0, [r1, #0]\n");
                }
                need('"');if(j<len){address(slot);emit("push {r0}\n");regimm("movs",0,j);emit("pop {r1}\nadds r0, r1\nmovs r1, #0\nstrb r1, [r0, #0]\n");}
            } else {if(is_array)failed=1;expression(0);
                if(ptr && value_ptr && ptr!=value_ptr)failed=1;
                if(!ptr && value_ptr)failed=1;
                if(type==1){emit("push {r0}\n");address(slot);emit("pop {r1}\nstrb r1, [r0, #0]\n");}
                else local_access("str",0,slot);}
        } else if(!is_array){regimm("movs",0,0);if(type==1){emit("push {r0}\n");address(slot);emit("pop {r1}\nstrb r1, [r0, #0]\n");}else local_access("str",0,slot);}
        need(';');return;
    }
    if(word("if")) {
        int end=labels++;need('(');condition(end);need(')');block();
        if(word("else")){int after=labels++;branch("b",after);mark(end);block();mark(after);}
        else mark(end);
        return;
    }
    if(word("while")) {
        int start=labels++,end=labels++;mark(start);need('(');condition(end);need(')');block();branch("b",start);mark(end);return;
    }
    if(token('*')) {
        /* *p = expr; : preserve target address while evaluating RHS. */
        unary();int size=value_ptr;
        if(!size)failed=1;
        emit("push {r0}\n");need('=');expression(0);
        if(value_ptr)failed=1;
        emit("pop {r1}\n");
        emit(size==1?"strb r0, [r1, #0]\n":"str r0, [r1, #0]\n");
        need(';');return;
    }
    ws();if(letter(source[at]) && strncmp(source+at,"write",5) && strncmp(source+at,"getpid",6) && strncmp(source+at,"return",6) && strncmp(source+at,"read",4)) {
        char name[20];identifier(name);ws();
        if(source[at]=='(') {function_call(name);need(';');return;}
        int slot=variable(name);
        int post=step_token();
        if(post) { incdec(slot,post,1); need(';'); return; }
        int indexed=token('[');if(indexed){at--; /* defer index parsing until RHS */}
        if(indexed){ /* preserve index address across expression evaluation */
            array_address(slot);emit("push {r0}\n");need('=');expression(0);
            if(value_ptr)failed=1;
            emit("pop {r1}\n");
            emit((ptrs[slot]?ptrs[slot]:types[slot])==1?"strb r0, [r1, #0]\n":"str r0, [r1, #0]\n");
        } else {need('=');expression(0);if(arrays[slot])failed=1;
            if(ptrs[slot] && value_ptr && ptrs[slot]!=value_ptr)failed=1;
            if(!ptrs[slot] && value_ptr)failed=1;
            if(types[slot]==1){emit("push {r0}\n");address(slot);emit("pop {r1}\nstrb r1, [r0, #0]\n");}
            else local_access("str",0,slot);}
        need(';');return;
    }
    if(word("read")) {syscall_buffer(8);need(';');return;}
    if(word("write")) {
        ws();
        /* Retain the literal form for the tiny assembler's ADR/.ascii path. */
        int save=at;
        need('(');
        int literal_form=0, fd=0;
        ws();
        if(digit(source[at])) {
            int start=at;
            while(digit(source[at]))at++;
            ws();
            if(source[at]==',') {
                at++;ws();literal_form=(source[at]=='"');
            }
            at=start;
        }
        if(!literal_form) {
            at=save;
            syscall_buffer(9);need(';');return;
        }
        fd=integer();need(',');need('"');
        char literal[256];int n=0,bytes=0;
        while(source[at]&&source[at]!='"'&&!failed) {
            char c=source[at++];
            if(c=='\\'){char e=source[at++];if(e!='n'&&e!='r'&&e!='t'&&e!='\\'){failed=1;break;}c='\\';if(n>=253){failed=1;break;}literal[n++]=c;c=e;}
            if(n>=253){failed=1;break;}literal[n++]=c;bytes++;
        }
        need('"');need(',');int count=integer();need(')');need(';');
        if(fd<0||fd>2||count<0||count>bytes)failed=1;
        if(failed)return;
        imm("movs r0,",9);imm("movs r1,",fd);
        char line[48];snprintf(line,sizeof line,"adr r2, s%d\n",labels);emit(line);
        imm("movs r3,",count);emit("svc #0\n");
        snprintf(line,sizeof line,"b e%d\n.align 2\ns%d:\n.ascii \"",labels,labels);emit(line);
        literal[n]=0;emit(literal);emit("\"\n.align 2\n");
        snprintf(line,sizeof line,"e%d:\n",labels++);emit(line);
    } else if(word("getpid")) {
        need('(');need(')');need(';');imm("movs r0,",1);emit("svc #0\n");
    } else if(word("return")) {
        if(!token(';')) {expression(0);need(';');}
        else if(!function_void)failed=1;
        branch("b",function_end);
    } else failed=1;
}
/* 入力Cソースを解析してThumbアセンブリへ変換する． */
static int compile(void) {
    /* Entry stub uses the normal process-exit syscall after main returns. */
    emit("bl f_main\nmov r1, r0\nmovs r0, #3\nsvc #0\n");
    while(!failed) {
        ws();if(!source[at])break;
        function_void=word("void");
        if(!function_void && !word("int")){failed=1;break;}
        char name[20];identifier(name);
        if(nfunctions>=16){failed=1;break;}
        for(int i=0;i<nfunctions;i++)if(!strcmp(functions[i],name))failed=1;
        if(!strcmp(name,"main")){if(function_void)failed=1;have_main=1;}
        strcpy(functions[nfunctions],name);
        char line[48];snprintf(line,sizeof line,"f_%s:\n",name);emit(line);
        nvars=0;frame_used=0;
        need('(');
        char params[4][20];int param_ptr[4];int count=0;
        if(!token(')')) {
            if(word("void"))need(')');
            else {
                do {
                    int param_type=word("int")?4:(word("char")?1:0);
                    if(!param_type || count>=4){failed=1;break;}
                    param_ptr[count]=token('*')?param_type:0;
                    if(!param_ptr[count] && param_type!=4)failed=1;
                    identifier(params[count++]);
                } while(token(','));
                need(')');
            }
        }
        for(int i=0;i<count;i++)param_kinds[nfunctions][i]=param_ptr[i];
        function_args[nfunctions++]=count;
        emit("push {r4, lr}\nsub sp, #256\naddsp r4, #0\n");
        for(int i=0;i<count;i++) {
            for(int j=0;j<i;j++)if(!strcmp(params[i],params[j]))failed=1;
            strcpy(vars[i],params[i]);types[i]=4;ptrs[i]=param_ptr[i];lengths[i]=1;arrays[i]=0;offsets[i]=i*4;
            snprintf(line,sizeof line,"str r%d, [sp, #%d]\n",i,i*4);emit(line);
        }
        nvars=count;frame_used=count*4;
        function_end=labels++;
        block();
        if(!function_void)emit("movs r0, #0\n");
        mark(function_end);
        emit("add sp, #256\npop {r4, pc}\n");
    }
    if(!have_main)failed=1;
    for(int i=0;i<ncalls;i++) {
        int found=0;
        for(int j=0;j<nfunctions;j++)
            if(!strcmp(calls[i],functions[j]) && call_args[i]==function_args[j]) {
                found=1;
                for(int k=0;k<call_args[i];k++)
                    if(call_kinds[i][k] && call_kinds[i][k]!=param_kinds[j][k])failed=1;
            }
        if(!found)failed=1;
    }
    return failed?-1:0;
}

/* Keep the assembly next to the source: foo.c -> foo.s, /tmp/foo.c -> /tmp/foo.s. */
static int assembly_path(const char *source_path,char *dest,int capacity) {
    int n=(int)strlen(source_path);
    if(n<3 || source_path[n-2]!='.' || source_path[n-1]!='c')return -1;
    if(n>=capacity)return -1;
    for(int i=0;i<n;i++)dest[i]=source_path[i];
    dest[n-1]='s';dest[n]=0;
    return 0;
}

/* 引数とファイルを処理しCコンパイルを実行する． */
int main(int argc,char **argv) {
    char dest[128];
    if(argc!=2){puts("usage: cc source.c  (writes source.s)");return 1;}
    if(assembly_path(argv[1],dest,sizeof dest)<0){puts("cc: expected a .c filename (path too long?)");return 1;}
    int fd=open(argv[1],O_RDONLY);if(fd<0){puts("cc: cannot open source");return 1;}
    int n=read(fd,source,SOURCE_MAX);char extra;int overflow=read(fd,&extra,1);close(fd);
    if(n<=0||overflow>0){puts("cc: source exceeds 1024 bytes or is empty");return 1;}
    source[n]=0;
    if(compile()<0){
        puts(output_overflow ? "cc: generated assembly exceeds internal buffer (4096 bytes)" :
                               "cc: unsupported C syntax or compiler limit");
        return 1;
    }
    if(used>ASM_FILE_MAX){
        puts("cc: generated assembly exceeds RAM file limit (2048 bytes)");
        return 1;
    }
    fd=open(dest,O_WRONLY|O_CREAT|O_TRUNC);
    if(fd<0){puts("cc: cannot create assembly file");return 1;}
    int ok=write(fd,output,(size_t)used)==used;close(fd);
    if(!ok){
        unlink(dest); /* never leave truncated assembly behind */
        puts("cc: cannot write complete assembly file (removed partial output)");
        return 1;
    }
    printf("cc: %d bytes -> %s\n",used,dest);return 0;
}
