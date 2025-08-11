// CPU用ジェネレータテーブル事前計算ユーティリティ
#include "SECP256k1.h"
#include "Point.h"
#include <stdio.h>

#define CPU_GRP_SIZE 1024 // 100万 key/s目標

int main() {
    fprintf(stderr, "Generating CPU generator table...\n");
    
    printf("// CPU Generator Table - Auto generated\n");
    printf("// G[n] = (n+1)*G for n = 0 to %d\n", CPU_GRP_SIZE/2-1);
    printf("static const uint64_t cpu_gtable_raw[%d][15] = {\n", CPU_GRP_SIZE/2);
    
    // 一時的にデバッグログを無効にして、純粋なテーブルのみ出力
    setvbuf(stdout, NULL, _IOLBF, 0); // 行バッファリング
    
    Secp256K1 secp;
    secp.Init();
    
    fprintf(stderr, "SECP256K1 initialized.\n");
    
    Point g = secp.G;
    Point Gn[CPU_GRP_SIZE/2];
    
    // 計算
    Gn[0] = g;
    printf("  // G[0] = G\n");
    printf("  {");
    for(int k = 0; k < 5; k++) printf("0x%016llxULL,", g.x.bits64[k]);
    for(int k = 0; k < 5; k++) printf("0x%016llxULL,", g.y.bits64[k]);
    for(int k = 0; k < 5; k++) printf("0x%016llxULL%s", g.z.bits64[k], k==4?"}":(","));
    printf(",\n");
    
    g = secp.DoubleDirect(g);
    Gn[1] = g;
    printf("  // G[1] = 2*G\n");
    printf("  {");
    for(int k = 0; k < 5; k++) printf("0x%016llxULL,", g.x.bits64[k]);
    for(int k = 0; k < 5; k++) printf("0x%016llxULL,", g.y.bits64[k]);
    for(int k = 0; k < 5; k++) printf("0x%016llxULL%s", g.z.bits64[k], k==4?"}":(","));
    printf(",\n");
    
    for(int i = 2; i < CPU_GRP_SIZE/2; i++) {
        g = secp.AddDirect(g, secp.G);
        Gn[i] = g;
        printf("  // G[%d] = %d*G\n", i, i+1);
        printf("  {");
        for(int k = 0; k < 5; k++) printf("0x%016llxULL,", g.x.bits64[k]);
        for(int k = 0; k < 5; k++) printf("0x%016llxULL,", g.y.bits64[k]);
        for(int k = 0; k < 5; k++) printf("0x%016llxULL%s", g.z.bits64[k], k==4?"}":(","));
        printf("%s\n", i == CPU_GRP_SIZE/2-1 ? "" : ",");
        
        if(i % 32 == 0) {
            fprintf(stderr, "Progress: %d/%d\n", i, CPU_GRP_SIZE/2);
        }
    }
    
    printf("};\n\n");
    
    // _2Gn も計算
    Point _2Gn = secp.DoubleDirect(Gn[CPU_GRP_SIZE/2-1]);
    printf("// _2Gn = CPU_GRP_SIZE*G\n");
    printf("static const uint64_t cpu_2gn_raw[15] = {\n");
    printf("  ");
    for(int k = 0; k < 5; k++) printf("0x%016llxULL,", _2Gn.x.bits64[k]);
    for(int k = 0; k < 5; k++) printf("0x%016llxULL,", _2Gn.y.bits64[k]);
    for(int k = 0; k < 5; k++) printf("0x%016llxULL%s", _2Gn.z.bits64[k], k==4?"":(","));
    printf("\n};\n");
    
    return 0;
}
