/* ------------------------------------------------------------- *
 *   INCLUDES / DEFINES / DECLARACOES                            *
 * ------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <SDL.h>

#include <SDL_touch.h>
#include <SDL_image.h>

#define SPRITE_SIZE_32  32
#define SPRITE_SIZE_64  64
#define OBJECT_SIZE     32
#define BG_SIZE         32
#define FREEZE_TIME     60
#define DELAY_SPRITE     6
#define BPP              4
#define MAX            999

/* ------------------------------------------------------------- *
 *   VARS GLOBAIS                                                *
 * ------------------------------------------------------------- */
// - propriedades da tela
int width;          // largura
int height;         // altura
// - adequacao ao tamanho dos objetos
int width32;        // largura (em blocos)
int height32;       // altura (em blocos)
int middleBX;       // posicao central (bloco)
// - quantidade de inimigos e obstaculos
int qtde_obj;
int qtde_inim;
// - booleano de fim do jogo
bool done = false;

SDL_Surface *screen;

/* ------------------------------------------------------------- *
 *   FUNCAO DE RANDOMIZACAO                                      *
 * ------------------------------------------------------------- */
inline int randomize() {
    int t = time(NULL);
    srand(t);
    return t;
}

inline void randomize(unsigned int seed) { srand(seed); }
void fun(char aef, char in);
inline double rnd(float max) { return max * rand() / (RAND_MAX + 1.0); }
inline double rnd(float min, float max) { return (rnd(max-min)+min); }

/* ------------------------------------------------------------- *
 *   CLASSES                                                     *
 * ------------------------------------------------------------- */
using namespace std;

/* -- Classe Bloco --------------------------------------------- */
class Bloco
{
    public:
        int x;
        int y;
        int curFrame_x;
        int curFrame_y;
        int frameWidth;
        int frameHeight;
        bool escondido;
        SDL_Surface *sprite_pic;
        SDL_Rect rcSprite;
        SDL_Rect rcSrc;

        void MoveX(int x);
        void MoveY(int y);
        void MostraBloco(int x, int y);
        void EscondeBloco();
        bool Escondido();
        bool DetectaColisao(int x1, int y1);
        void MudaFrame(int curFrame_x, int curFrame_y);
        void Inicializa(char *path, int x, int y, int frameWidth, int frameHeight);
};

void Bloco::MoveX(int x)
{
    this->x = x;
    this->rcSprite.x = x;
}

void Bloco::MoveY(int y)
{
    this->y = y;
    this->rcSprite.y = y;
}

void Bloco::MostraBloco(int x, int y) {
    this->x = x;
    this->rcSprite.x = x;
    this->y = y;
    this->rcSprite.y = y;
    this->escondido = false;
}

void Bloco::EscondeBloco() {
    this->x = width+100;
    this->rcSprite.x = width+100;
    this->y = height+100;
    this->rcSprite.y = height+100;
    this->escondido = true;
}

bool Bloco::Escondido() {
    return (this->escondido) ? true : false;
}

void Bloco::MudaFrame(int curFrame_x, int curFrame_y)
{
    this->curFrame_x = curFrame_x;
    this->curFrame_y = curFrame_y;
    this->rcSrc.x = this->curFrame_x * this->frameWidth;
    this->rcSrc.y = this->curFrame_y * this->frameHeight;

}

bool Bloco::DetectaColisao(int x1, int y1) {
    int xCentro = (int) (this->rcSprite.x + (OBJECT_SIZE / 2));
    int yCentro = (int) (this->rcSprite.y + (OBJECT_SIZE / 2));

    if ( (x1 < xCentro) && ((x1+OBJECT_SIZE) > xCentro) &&
         (y1 < yCentro) && ((y1+OBJECT_SIZE) > yCentro) )
    {
        return true;
    }
    return false;
}

void Bloco::Inicializa(char *path, int x, int y, int frameWidth, int frameHeight)
{
    SDL_Surface *image;
    image = IMG_Load(path);
    this->escondido = false;
    this->sprite_pic = image; // SDL_LoadBMP(path);
    this->x = x;
    this->y = y;
    this->rcSrc.x = y;
    this->rcSrc.y = x;
    this->rcSrc.w = frameWidth;
    this->rcSrc.h = frameHeight;
    this->frameWidth = frameWidth;
    this->frameHeight = frameHeight;
    this->rcSprite.x = 0;
    this->rcSprite.y = 0;
    this->rcSprite.w = frameWidth;
    this->rcSprite.h = frameHeight;
}

// @declaracao_obstaculos
Bloco bl_ice[MAX];

/* -- Classe Fundo --------------------------------------------- */
class Fundo
{
    public:
        SDL_Surface *sprite_pic;
        SDL_Rect rcGrass;
        void Inicializa(char *path);
};

void Fundo::Inicializa(char *path)
{
    this->sprite_pic = SDL_LoadBMP( path );
}

/* -- Classe Sprite -------------------------------------------- */
class Sprite
{
    public:
        int x;
        int y;
        int frameCount;
        int curFrame;
        int frameWidth;
        int frameHeight;
        int velocidadeX;
        bool escondido;
        bool direcao; // 0-esq ; 1-dir
        //Imagem com a folha de sprite
        SDL_Surface *sprite_pic;
        SDL_Rect rcSprite;
        SDL_Rect rcSrc;
        void MovimentaSprite();
        void MudaFrame(int posicao); // 0-esq ; 1-dir; 2-cima; 3-baixo
        void AtualizaSprite();
        void AtualizaSprites();
        void MoveX(int x);
        void MoveY(int y);
        bool DetectaColisao(Sprite sprite);
        bool DetectaColisao(Bloco bloco);
        void MostraSprite(int x, int y);
        void EscondeSprite();
        bool Escondido();
        void Inicializa(int x, int y,
                        int velocidade, bool direcao,
                        int framecount, int tamX, int tamY,
                        char *sprite_pic_file);
};

void Sprite::MovimentaSprite()
{
    this->MoveX(0);
}
void Sprite::MoveX(int x)
{
    // verifica se passo manual ou autom.
    int desloc = (x==0)
        ? this->velocidadeX : x;
    if (this->direcao == 0) {
        this->x -= desloc;
        this->rcSprite.x -= desloc;
    } else {
        this->x += desloc;
        this->rcSprite.x += desloc;
    }

    // controle de colisao com o final da tela
    if (this->x < 0)
    {
        this->direcao = 1;
        this->x = 0;
        this->rcSprite.x = 0;
    }
    else if (this->x > width - (this->frameWidth*2)) {
        this->direcao = 0;
        this->x = width - this->frameWidth*2;
        this->rcSprite.x = width - this->frameWidth*2;
    }

}
void Sprite::MoveY(int y)
{
    // move
    this->y += y;
    this->rcSprite.y += y;

    // controle de colisao com o final da tela
    if (this->y < 0) {
        this->y = 0;
        this->rcSprite.y = 0;
    }
    else if (this->y > height - this->frameHeight) {
        this->y = height - this->frameHeight;
        this->rcSprite.y = height - this->frameHeight;
    }
}

void Sprite::MudaFrame(int pos)
{
    switch(pos) {
        case 0: // esq
            this->curFrame = 4;
            break;
        case 1: // dir
            this->curFrame = 6;
            break;
        case 2: // cima
            this->curFrame = 0;
            break;
        case 3: // baixo
            this->curFrame = 2;
            break;
    }
}
void Sprite::AtualizaSprite() {
    // itera frame do sprite personagem
    switch (this->curFrame) {
        case 0: this->curFrame = 1; break;
        case 1: this->curFrame = 0; break;
        case 2: this->curFrame = 3; break;
        case 3: this->curFrame = 2; break;
        case 4: this->curFrame = 5; break;
        case 5: this->curFrame = 4; break;
        case 6: this->curFrame = 7; break;
        case 7: this->curFrame = 6; break;
    }
    this->rcSrc.x = this->curFrame * this->frameWidth;
}
void Sprite::AtualizaSprites() {
    // itera frame do sprite dos inimigos
    this->curFrame++;
    if (this->curFrame > (this->frameCount-1))
        this->curFrame = 0;
    this->rcSrc.x = this->curFrame * this->frameWidth;
}

bool Sprite::DetectaColisao(Sprite sp) {
    int xCentro = (int) (sp.rcSprite.x + (sp.frameWidth  / 2));
    int yCentro = (int) (sp.rcSprite.y + (sp.frameHeight / 2));

    if ( (this->rcSprite.x < xCentro) && ((this->rcSprite.x + this->frameWidth)  > xCentro) &&
         (this->rcSprite.y < yCentro) && ((this->rcSprite.y + this->frameHeight) > yCentro) )
    {
        return true;
    }
    return false;
}

bool Sprite::DetectaColisao(Bloco bl) {
    int xCentro = (int) (bl.rcSprite.x + (OBJECT_SIZE / 2));
    int yCentro = (int) (bl.rcSprite.y + (OBJECT_SIZE / 2));

    if ( (this->rcSprite.x < xCentro) && ((this->rcSprite.x + this->frameWidth)  > xCentro) &&
         (this->rcSprite.y < yCentro) && ((this->rcSprite.y + this->frameHeight) > yCentro) )
    {
        return true;
    }
    return false;
}

void Sprite::MostraSprite(int x, int y) {
    this->x = x;
    this->rcSprite.x = x;
    this->y = y;
    this->rcSprite.y = y;
    this->escondido = false;
}

void Sprite::EscondeSprite() {
    this->x = width+100;
    this->rcSprite.x = width+100;
    this->y = height+100;
    this->rcSprite.y = height+100;
    this->escondido = true;
}

bool Sprite::Escondido() {
    return (this->escondido) ? true : false;
}

void Sprite::Inicializa(int x, int y, int vel, bool dir, int framecount, int tamX, int tamY, char *sprite_pic_file)
{
    this->x = x;
    this->y = y;
    this->direcao = dir;
    this->velocidadeX = vel;
    this->frameCount = framecount;
    this->curFrame = 1;
    this->frameWidth = tamX;
    this->frameHeight = tamY;
    this->sprite_pic = SDL_LoadBMP(sprite_pic_file);
    this->rcSprite.w = frameWidth;
    this->rcSprite.h = frameHeight;
    this->rcSrc.x = 0;
    this->rcSrc.y = 0;
    this->rcSrc.w = tamX;
    this->rcSrc.h = tamY;
    this->escondido = false;

}

// @declaracao_sprite_principal
Sprite spr_hero;

/* -- Funcoes de Desenho --------------------------------------- */
void setpix(SDL_Surface *screen, float _x, float _y, unsigned int col)
{
    Uint32 *pixmem32;
    Uint32 colour;
    Uint8 r, g, b;
    int x = (int)_x;
    int y = (int)_y;
    float a;

    if (x < 0 || x >= screen->w) return;
    if (y < 0 || y >= screen->h) return;

    pixmem32 = (Uint32*)screen->pixels + y*screen->pitch / BPP + x;

    SDL_memcpy(&colour, pixmem32, screen->format->BytesPerPixel);

    SDL_GetRGB(colour, screen->format, &r, &g, &b);
    //r = 0;g = 0; b = 0;
    a = (float)((col >> 24) & 0xFF);
    if (a == 0) a = 0xFF; //Hack, to make things easier.
    a /= 0xFF;
    r = (Uint8)(r*(1 - a) + ((col >> 16) & 0xFF)*(a));
    g = (Uint8)(g*(1 - a) + ((col >> 8) & 0xFF)*(a));
    b = (Uint8)(b*(1 - a) + ((col >> 0) & 0xFF)*(a));
    colour = SDL_MapRGB(screen->format, r, g, b);

    *pixmem32 = colour;
}

void drawCircle(SDL_Surface* screen, float x, float y, float r, unsigned int c)
{
    float tx, ty;
    float xr;
    for (ty = (float)-SDL_fabs(r); ty <= (float)SDL_fabs((int)r); ty++) {
        xr = (float)sqrt(r*r - ty*ty);
        if (r > 0) { //r > 0 ==> filled circle
            for (tx = -xr + .5f; tx <= xr - .5; tx++) {
                setpix(screen, x + tx, y + ty, c);
            }
        }
        else {
            setpix(screen, x - xr + .5f, y + ty, c);
            setpix(screen, x + xr - .5f, y + ty, c);
        }
    }
}

/* -- Acoes e eventos do Touch --------------------------------- */
void HandleEvent(SDL_Event event)
{
    int x1, y1;
    int hx, hy;
    int mf;
    bool anda;

    switch (event.type) {

    /*
        Acoes:
        SDL_FINGERMOTION
        SDL_FINGERDOWN
        SDL_FINGERUP
    */

    case SDL_FINGERDOWN:

        // captura posicao do clique
        x1 = event.tfinger.x * screen->w;
        y1 = event.tfinger.y * screen->h;

        // guarda posicao do personagem
        hx = spr_hero.rcSprite.x;
        hy = spr_hero.rcSprite.y;

        // captura touch
        // -- move direita
        if ((hx < x1) && ((hx+SPRITE_SIZE_32) < x1)) {
            hx += SPRITE_SIZE_32;
            mf=1;
        }

        // -- move esquerda
        if (hx > x1) {
            hx -= SPRITE_SIZE_32;
            mf=0;
        }

        // -- move cima
        if (hy > y1) {
            hy -= SPRITE_SIZE_32;
            mf=2;
        }

        // -- move baixo
        if ((hy < y1) && ((hy+SPRITE_SIZE_32) < y1)) {
            hy += SPRITE_SIZE_32;
            mf=3;
        }

        // - verifica se passo esta livre (nao tem obstaculo)
        anda=true;
        for (int i=0; i<qtde_obj; i++) {
            if ( bl_ice[i].DetectaColisao(hx, hy) )
                anda=false;
        }
        // - se livre segue em frente
        if (anda) {
            spr_hero.rcSprite.x = hx;
            spr_hero.rcSprite.y = hy;
            spr_hero.MudaFrame(mf);
            spr_hero.AtualizaSprite();
        }
        break;

    /* sair do jogo */
    case SDL_QUIT:
        done = true;
        break;

    }
}

void ReiniciaJogo() {
    spr_hero.x = middleBX;
    spr_hero.rcSprite.x = middleBX;
    spr_hero.y = 0;
    spr_hero.rcSprite.y = 0;
}

/* ------------------------------------------------------------- *
 *   PROGRAMA PRINCIPAL                                          *
 * ------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    SDL_Window* window = 0;
    SDL_GLContext gl = 0;

    /* -- Inicializa SDL --------------------------------------- */
    if (0 != SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // - Propriedades do Display
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);
    width     = mode.w;
    height    = mode.h;
    // - propriedades de tamanho blocado
    width32   = (int)(width/32);
    height32  = (int)(height/32);
    middleBX  = ((int)(width32/2))*OBJECT_SIZE;
    // - quantidade de inimigos e obstaculos na tela,
    // - de acordo com dimensoes
    qtde_inim = (int)((height32-3)/2);
    qtde_obj  = (qtde_inim > (MAX/3)) ? MAX : (qtde_inim * 3);

    SDL_Log("Width = %d, Height = %d\n", width, height);

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /* -- Cria janela do jogo ---------------------------------- */
    window = SDL_CreateWindow("Jogo", 0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE);
    if (window == 0) {
        SDL_Log("Could not initialize Window\n");
        SDL_Quit();
        return 1;
    }

    screen = SDL_GetWindowSurface(window);

    /* -- Construcao dos objetos do jogo ----------------------- */
    char path_bl_flag_red[] = "obj/obj_flag_red.png";
    char path_bl_flag_ini[] = "obj/obj_start.png";
    char path_bl_ice[]      = "obj/obj_ice2.png";
    char path_bl_start[]    = "txt/start2.png";
    char path_bl_win[]      = "txt/win.png";

    Bloco bl_flag_red = Bloco();
    Bloco bl_flag_ini = Bloco();
    Bloco bl_start    = Bloco();
    Bloco bl_win      = Bloco();

    // - inicializacao dos objetos
    // -- void Inicializa(char *path, int x, int y, int frameWidth, int frameHeight);
    bl_flag_red.Inicializa(path_bl_flag_red, 0, 0, OBJECT_SIZE, OBJECT_SIZE);
    bl_flag_ini.Inicializa(path_bl_flag_ini, 0, 0, OBJECT_SIZE, OBJECT_SIZE);
    bl_start.Inicializa(path_bl_start, 0, 0, 300, 60);
    bl_win.Inicializa(path_bl_win, 0, 0, 430, 60);

    //b1.MudaFrame(0,0);

    // - posicao das bandeiras
    int posFlag = ((int)(height32-2))*32;
    bl_flag_red.MoveX(middleBX);
    bl_flag_red.MoveY(posFlag);
    bl_flag_ini.MoveX(middleBX);
    bl_flag_ini.MoveY(0);

    // - posicao do texto start
    int start_x_ini = (int)((width/2)-(300/2)); // centralizado x
    int start_y_ini = (int)((height/2)-(60/2)); // centralizado y
    bl_start.MoveX(start_x_ini);
    bl_start.MoveY(start_y_ini);

    // - posicao do texto win
    int win_x_ini = (int)((width/2)-(430/2));
    int win_y_ini = (int)((height/2)-(60/2));
    bl_win.MoveX(win_x_ini);
    bl_win.MoveY(win_y_ini);

    // - inicializacao dos obstaculos
    int j=0;
    int x_rnd;
    for (int i=0; i<qtde_obj; i++) {
        x_rnd=(int)(((int)rnd(width32-1))*OBJECT_SIZE);
        bl_ice[i] = Bloco();
        bl_ice[i].Inicializa(path_bl_ice, 0, 0, OBJECT_SIZE, OBJECT_SIZE);
        bl_ice[i].MoveX(x_rnd);
        bl_ice[i].MoveY(((j*2)+1)*OBJECT_SIZE);
        if (i%3 == 2) j++;
    }


    /* -- Construcao do Fundo ---------------------------------- */
    Fundo bg_tile = Fundo();
    char path_bg[] = "bg/bg_ice.bmp";
    // -- void Inicializa(char *path);
    bg_tile.Inicializa(path_bg);


    /* -- Construcao dos Personagens e Inimigos ---------------- */
    spr_hero = Sprite();
    Sprite spr_obj1 = Sprite();
    Sprite spr_bad[qtde_inim];

    // - Seta nome do arquivo dos sprites
    char path_spr_hero[] = "spr/spr_king.bmp";
    char path_spr_bad1[] = "spr/spr_ghost.bmp";
    char path_spr_bad2[] = "spr/spr_arrow.bmp";
    char path_spr_bad3[] = "spr/spr_alpha.bmp";
    char path_spr_bad4[] = "spr/spr_shuriken.bmp";
    char path_spr_obj1[] = "spr/spr_coin.bmp";

    // - Inicializa sprites
    // -- void Inicializa(int x, int y, int velocidade, int direcao, int framecount, char *sprite_pic_file);
    spr_hero.Inicializa(0, 0, 10, 1, 8, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_hero);
    spr_obj1.Inicializa(0, 0, 05, 0, 8, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_obj1);

    // - Configura o controle de cores dos sprites
    int colorkey = SDL_MapRGB(screen->format, 255, 0, 255);
    SDL_SetColorKey(spr_hero.sprite_pic, SDL_TRUE, colorkey);
    SDL_SetColorKey(spr_obj1.sprite_pic, SDL_TRUE, colorkey);

    // - Posiciona sprites
    spr_hero.MudaFrame(3);
    spr_hero.MoveX(middleBX);
    spr_hero.MoveY(0);

    int obj1X = 4*OBJECT_SIZE;
    int obj1Y = 9*OBJECT_SIZE;
    spr_obj1.MoveX(obj1X);
    spr_obj1.MoveY(obj1Y);
    spr_obj1.MoveX(obj1X);

    // - inicializa inimigos (4 niveis dificuldade)
    int k=0;
    int v_rnd;
    for (int i=0; i<qtde_inim; i++) {
        spr_bad[i] = Sprite();
        switch (k) {
            case 0:
                v_rnd=(int)rnd(((k*10)+1) , ((k+1)*10));
                spr_bad[i].Inicializa(0, 0, v_rnd, (i%2), 8, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_bad1);
                break;
            case 1:
                v_rnd=(int)rnd(((k*10)+1) , ((k+1)*10));
                spr_bad[i].Inicializa(0, 0, v_rnd, (i%2), 8, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_bad2);
                break;
            case 2:
                v_rnd=(int)rnd(((k*10)+1) , ((k+1)*10));
                spr_bad[i].Inicializa(0, 0, v_rnd, (i%2), 8, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_bad3);
                break;
            case 3:
                v_rnd=(int)rnd(((k*10)+1) , ((k+1)*10));
                spr_bad[i].Inicializa(0, 0, v_rnd, (i%2), 4, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_bad4);
                break;
            default:
                v_rnd=(int)rnd(1,50);
                spr_bad[i].Inicializa(0, 0, v_rnd, (i%2), 8, SPRITE_SIZE_32, SPRITE_SIZE_32, path_spr_bad1);
                break;
        }
        x_rnd=(int)(rnd(1,width32)*SPRITE_SIZE_32);
        spr_bad[i].MoveX(x_rnd);
        spr_bad[i].MoveY((i+1)*2*SPRITE_SIZE_32);
        SDL_SetColorKey(spr_bad[i].sprite_pic, SDL_TRUE, colorkey);
        if (i%4 == 3) k++;
    }

    /* -- LOOP DE ANIMACAO DO JOGO ----------------------------- */
    done = false;
    bool freeze = false;
    int delay = 0;
    int delayFreeze = 0;
    int delayStart = 30;
    int delayQuit = 80;
    bl_win.EscondeBloco();
    while (1)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event) and (!done))
        {
            HandleEvent(event);
        }

        // - Desenha o fundo
        for (int x = 0; x < width / BG_SIZE; x++) {
            for (int y = 0; y < height / BG_SIZE; y++) {
                bg_tile.rcGrass.x = x * BG_SIZE;
                bg_tile.rcGrass.y = y * BG_SIZE;
                SDL_BlitSurface(bg_tile.sprite_pic, NULL, screen, &bg_tile.rcGrass);
            }
        }

        // - Desenha blocos
        SDL_BlitSurface(bl_flag_red.sprite_pic, &bl_flag_red.rcSrc, screen, &bl_flag_red.rcSprite);
        SDL_BlitSurface(bl_flag_ini.sprite_pic, &bl_flag_ini.rcSrc, screen, &bl_flag_ini.rcSprite);
        for (int i=0; i<qtde_obj; i++)
            SDL_BlitSurface(bl_ice[i].sprite_pic, &bl_ice[i].rcSrc, screen, &bl_ice[i].rcSprite);

        // - Desenha textos
        if (!bl_start.Escondido()) SDL_BlitSurface(bl_start.sprite_pic, &bl_start.rcSrc, screen, &bl_start.rcSprite);
        if (!bl_win.Escondido()) SDL_BlitSurface(bl_win.sprite_pic, &bl_win.rcSrc, screen, &bl_win.rcSprite);

        // 0xFFFFFF - desenha um circulo
        //drawCircle(screen, 50, 50, 30, 0xFFD700);

        // - Anima Sprites (delay de 5 frames para animar)
        if (delay > DELAY_SPRITE) {
            spr_hero.AtualizaSprite();
            delay=0;
        } else delay++;

        if (delayStart == 0) {
            bl_start.EscondeBloco();
        } else delayStart--;

        // - Desenha sprites
        SDL_BlitSurface(spr_hero.sprite_pic, &spr_hero.rcSrc, screen, &spr_hero.rcSprite);
        SDL_BlitSurface(spr_obj1.sprite_pic, &spr_obj1.rcSrc, screen, &spr_obj1.rcSprite);

        // - Animacao e tratamento dos sprites
        for (int i=0; i<qtde_inim; i++) {
            if (!freeze) spr_bad[i].MovimentaSprite();
            spr_bad[i].AtualizaSprites();
            if ( spr_hero.DetectaColisao(spr_bad[i]) ) {
                ReiniciaJogo();
                spr_obj1.MostraSprite(obj1X, obj1Y);
                delayFreeze=0;
                freeze = false;
            }
            SDL_BlitSurface(spr_bad[i].sprite_pic, &spr_bad[i].rcSrc, screen, &spr_bad[i].rcSprite);
        }
        // - Verifica se pegou objeto para congelar inimigos temporariamente
        if (!freeze) {
            if ( spr_hero.DetectaColisao(spr_obj1) ) {
                spr_obj1.EscondeSprite();
                freeze = true;
            }
        } else {
            if (delayFreeze > FREEZE_TIME) {
                delayFreeze=0;
                freeze = false;
            } else delayFreeze++;
        }
        spr_obj1.AtualizaSprites();

        // - Refresh
        SDL_UpdateWindowSurface(window);
        SDL_GL_SwapWindow(window);

        // - Detecta fim de jogo
        if ( spr_hero.DetectaColisao(bl_flag_red) ) {
            bl_win.MostraBloco(win_x_ini, win_y_ini);
            done = true;
        }

        // - timing para sair do jogo
        if (done) delayQuit--;
        if (delayQuit == 0) break;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return true;
}
