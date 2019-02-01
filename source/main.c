//プチコンぽい感じにスプライトを扱うデモ
//2019/02/01 Dai Matsmoto

#include <citro2d.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SPRITES   65535
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

typedef struct
{
	C2D_Sprite spr;
  int usingflg;           //flag of 'spset'
  int showflg;            //flag of 'spshow'
  int px,py;              //sprite's position
  float hx,hy;            //sprite's home
  int defnum;             //sprite's image number
  int animateflg;         //flag&counter of animation
  int animate_defnums[32];//imageNumbers array
  int animate_start;      //startTime of animation;
  int animate_flame;      //segment
  int animate_loop;       //flag of animation loop
} Sprite;

static C2D_SpriteSheet spriteSheet[1];
static Sprite sprites[MAX_SPRITES];
static int flameCntForSpritesAnimation = 0;//master counter

//全てのスプライトを初期化し非表示にします
static void initSprites() {
	size_t numImages = C2D_SpriteSheetCount(spriteSheet[0]);

	for (size_t i = 0; i < MAX_SPRITES; i++)
	{
		Sprite* sprite = &sprites[i];

		C2D_SpriteFromSheet(&sprite->spr, spriteSheet[0], 0);
		C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&sprite->spr, 0, 0);
    sprite->defnum   = 0;
    sprite->showflg  = 0;
    sprite->usingflg = 0;
    sprite->px = 0;
    sprite->py = 0;
    sprite->animateflg    = 0;
    sprite->animate_start = 0;
    sprite->animate_flame = 0;
  }
}

//指定番号のスプライトを有効化します
static void spset(int spnum, int defnum){
  if (spnum<MAX_SPRITES){
    Sprite* sprite = &sprites[spnum];
    C2D_SpriteFromSheet(&sprite->spr, spriteSheet[0], defnum);
    sprite->showflg = 1;
    sprite->usingflg = 1;
  }
}

//指定番号のスプライトの座標を変更します
static void spofs(int spnum, int x, int y){
  if (spnum<MAX_SPRITES && sprites[spnum].usingflg == 1){
    sprites[spnum].px = x;
    sprites[spnum].py = y;
  }
}

//指定番号のスプライトの中心を変更します
//0.5f,0.5fで画像の中央をスプライトの中心とします
static void sphome(int spnum, int x, int y){
  if (spnum<MAX_SPRITES && sprites[spnum].usingflg == 1){
    sprites[spnum].hx = x;
    sprites[spnum].hy = y;
  }
}

//指定番号のスプライトを表示/非表示にします
static void sphide(int spnum){
  if (sprites[spnum].usingflg == 1)
    sprites[spnum].showflg = 0;
}
static void spshow(int spnum){
  if (sprites[spnum].usingflg == 1)
    sprites[spnum].showflg = 1;
}

//指定番号のスプライトに対して
//defnums[]に渡した番号の画像を[flame]フレーム間隔でアニメーションさせます
//loopflgが1だとアニメーションはループをし続けます
static void spanim(int spnum, int defnums[], int flame, int loopflg){
  int flg = -1;
  Sprite* sprite = &sprites[spnum];
  if (sprite->usingflg ==1){
    sprite->animate_start = flameCntForSpritesAnimation;
    sprite->animate_flame = flame;
    sprite->animate_loop  = loopflg;
    for(int i=0;i<32;i++){
      if (defnums[i]==-1 && flg==-1)
        flg = i;
      if (flg == -1){
        sprite->animate_defnums[i] = defnums[i];
      }else{
        sprite->animate_defnums[i] = -1;
      }
    }
    sprite->animateflg = (flg-1)*flame;
  }
}

//spofsやspanimで設定したパラメータを実際に反映させます
//毎フレーム実行
static void SyncSpritesFromParamator() {
  int time;
  flameCntForSpritesAnimation ++;
	for (size_t i = 0; i < MAX_SPRITES; i++)
	{
  	Sprite* sprite = &sprites[i];
    if (sprite->usingflg == 1){

      //アニメーション
      if (sprite->animateflg > 0){
        sprite->animateflg --;
        time = flameCntForSpritesAnimation - sprite->animate_start;
        if (time % sprite->animate_flame == 0){
          C2D_SpriteFromSheet(&sprite->spr, spriteSheet[0], sprite->animate_defnums[time/sprite->animate_flame]);
        }
        if (sprite->animateflg == 0 && sprite->animate_loop == 1){
          int j=0;
          for(j=0;j<32;j++){
            if (sprite->animate_defnums[j] == -1) break;
          }
          sprite->animateflg = sprite->animate_flame * j;
          sprite->animate_start = flameCntForSpritesAnimation+sprite->animate_flame;
        }
      }

      //座標
      C2D_SpriteSetCenter(&sprite->spr, sprite->hx, sprite->hy);
      C2D_SpriteSetPos(&sprite->spr, sprite->px, sprite->py);
    }
  }
}

int main(int argc, char* argv[]) {
  //初期化とターゲット,リソースの確保
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	spriteSheet[0] = C2D_SpriteSheetLoad("romfs:/sprites.t3x");
	if (!spriteSheet[0]) svcBreak(USERBREAK_PANIC);
	initSprites();

  int ActivSprites = 0;
  int ActivToggle  = 0;
  int FPS = 0;
  int sec = 0;
  int dif = 0;

  //テスト
  int buf[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1};
  int t=0,u=0;
  for(t=0;t<MAX_SPRITES;t++){
    spset(t,0);
    sphome(t, 0.5f, 0.5f);
    spofs(t, rand()%400, rand()%240);
    spanim(t, buf, 1, 1);
  }

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

    if (kDown & KEY_A){
      if (ActivToggle == 0){
        for(t=0;t<MAX_SPRITES-256;t++)
          sphide(t);
        ActivToggle = 1;
      }else{
        for(t=0;t<MAX_SPRITES-256;t++)
          spshow(t);
        ActivToggle = 0;
      }
    }

    printf("\x1b[02;1HSPRITE TEST\n");
    printf("\x1b[27;1HMaterials borrowed by Pipoya.\n");
    printf("\x1b[28;1Hhttp://blog.pipoya.net\n");

    SyncSpritesFromParamator();
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top, C2D_Color32(0x00, 0xA0, 0x20, 0xFF));
		C2D_SceneBegin(top);
		for (size_t i = 0; i < MAX_SPRITES; i ++)
      if (sprites[i].showflg == 1 && sprites[i].usingflg == 1){
  			C2D_DrawSprite(&sprites[i].spr);
        ActivSprites++;
      }
		C3D_FrameEnd(0);

    printf("\x1b[05;1HActive sprites:% 6d\n",ActivSprites);
    ActivSprites = 0;

    //FPS計測(暫定FPS)
    sec = osGetTime() % 1000;
    FPS++;
    if (dif>sec){
      printf("\x1b[08;1H\x1b[32m\x1b[1mFPS:%02d\n\x1b[0m",FPS);
      FPS=0;
    }
    dif=sec;

	}

	// Delete graphics
	C2D_SpriteSheetFree(spriteSheet[0]);

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}
