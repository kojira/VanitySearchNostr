## 目的
Apple Silicon (arm64) 最適化で Nostr npub バニティマイニングのスループットを大幅向上し、`https://github.com/grunch/rana` と同等以上（毎秒数十万Key以上）を目指す。起動即採掘（初期化0秒）、詳細な進捗/プロファイル出力、厳格な入力バリデーションを維持。

## 現状サマリ
- **ビルド**: Apple Silicon 向けCPUビルド成功。`libsecp256k1`連携・`STATIC_GTABLE`フラグ対応済み。
- **初期化**: 実行時ログが「Initializing secp256k1...」から進まないケースあり。`STATIC_GTABLE`の即時ロードメッセージが出ないため、静的テーブル経路が動作していない可能性。
- **計測**: ハッシュレート表示に未到達（採掘開始前にブロック）。Rana超えは未達。
- **主要編集点（抜粋）**:
  - `Makefile`: `USE_LIBSECP256K1`/`STATIC_GTABLE`対応、arm64最適化、NEON RIPEMD-160追加ファイル。
  - `SECP256K1.cpp`: `ComputePublicKey`で`libsecp256k1`ブリッジ利用（定義時）。`NextKey`を`Add2`へ（逆元回避）。GTableのキャッシュ/並列生成/静的同梱経路を実装。
  - `secp256k1_bridge.cpp`: `libsecp256k1`でX/Y復元→`Point`へ格納。
  - `hash/*`: x86 asmローテートをポータブル化。CommonCrypto(SHA-256)統合（Apple Silicon）。
  - 進捗/プロファイル: `VS_PROGRESS`/`VS_PROFILE`でログ出力。

## ビルド/実行メモ
- 依存: Homebrew `secp256k1`
  ```bash
  brew install secp256k1
  ```
- ビルド（CPU, libsecp256k1, 静的GTable）
  ```bash
  USE_LIBSECP256K1=1 STATIC_GTABLE=1 PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig make cpu | cat
  ```
- 実行（8スレ、進捗/プロファイル有効、10〜30秒計測例）
  ```bash
  VS_PROGRESS=1 VS_PROFILE=1 ./VanitySearch -t 8 -stop 'npub1q*'
  ```

## 直近の問題と切り分け方針
- 症状（修正済み）: 「Initializing secp256k1...」→「Computing optimized generator table...」でハング。
- 現在の状況（2024-12-29更新）: STATIC_GTABLEは正常動作確認済み。フィールド設定修正により「SetupField computing Montgomery powers...」まで進行。Montgomery powers計算でハング中。
- 修正済み項目:
  - Apple Silicon対応: sha512.cppのx86アセンブリをポータブル実装に変更
  - NEON RIPEMD-160: 関数名の不一致を修正、型をunsigned charに統一
  - ビルドエラー: すべて解消済み
  - STATIC_GTABLE動作: 正常に読み込み完了（初期化0秒達成）
- 残存問題:
  - CPUジェネレータテーブル計算でのハング（Vanity.cpp:376付近のAddDirect呼び出し）
- 切り分け:
  1) `SECP256K1::Init()` 先頭と`#ifdef STATIC_GTABLE`入場直後にガードログを追加し、どの行で停止か特定。
  2) `#ifdef STATIC_GTABLE`が通っているか、コンパイル時/実行時に明示ログを出力。
  3) `k1_gtable.v1.inc` の配列名が `k1_gtable_raw`、次元が `[256*32][15]` で`NB64BLOCK==5`と一致するか確認。
  4) 一時的に `tryLoadCache()` を先呼びに変更するか、`VS_K1_CACHE=k1_gtable.v1.bin` を指定してキャッシュロード経路の稼働確認。

## 予定していた最適化タスク（優先順）
1. STATIC_GTABLEの確実化（必須）
   - `-DSTATIC_GTABLE`伝播確認、`k1_gtable.v1.inc`の配列名/次元照合、ロード完了ログ出力後`return`まで到達を確認。
   - 代替としてキャッシュロード経路の堅牢化（ヘッダ整合、チェックサム監査）。

2. RIPEMD-160 NEON最適化の本実装
   - 現状はスカララッパー。NEON 4-way並列で`ripemd160_neon_32x4`を完成。
   - 期待: 2〜3倍向上（ハッシュ部）。

3. `libsecp256k1`活用範囲の拡大
   - 現状は`ComputePublicKey`のみ。連番キー生成は`Add2`（Jacobian加算）で自前。
   - ボトルネックに応じてwNAF/endomorphism相当の適用可能性を検討（公開API範囲）。

4. エンドモルフィズムの再有効化（Nostr x-only向けに正しい鍵再構成）
   - `wrong private key`警告の根絶。
   - 期待: 1.8〜2.0倍。

5. GTableの大型化（ウィンドウ幅拡大）
   - 足し算回数を削減。静的同梱で起動0秒を維持。

6. フィールド演算のNEON化＋Montgomery trick（一括逆元）
   - 現行はJacobianで逆元頻度は低いが、他箇所の逆元/演算を削減。

## 進捗/ログ確認テンプレ
コンテキストが溢れてしまうため、必ずログファイルに保存してから中身を確認すること
```bash
VS_PROFILE=1 VS_PROGRESS=1 ./VanitySearch -t 8 -stop 'npub1q*' > bench.log 2>&1 &
sleep 15; pkill -f VanitySearch; tail -n 100 bench.log | cat; tail -n 200 /tmp/vanity_nostr.log | cat
```

## 期待スループットと評価方法
- 目標: 毎秒数十万Key以上（Rana同等以上）。
- 評価: 3〜10文字の`npub`ワイルドカードで10〜30秒ラン、`/tmp/vanity_nostr.log`の計測行からKey/sを集計。単位（K/M/G）自動表示の整合も確認。

## 既知の注意点
- 入力バリデーション: `npub`以外/Bech32不正文字で即エラー終了。
- 進捗の即時表示: `main.cpp`で行バッファリング解除済み。初期化内にも`fflush(stdout)`を適宜挿入すると確実。
- マルチプロセスGTable生成: macOSはCPUバウンドでMP有利。`VS_K1_PROCS` で制御（テーブル生成時のみ）。
- `wrong private key`警告: Nostr向けに再計算経路をスキップ済み。エンドモルフィズム再導入時に再検証。

## 直後にやるべき最小手順（引き継ぎ用）
1. Montgomery powers計算のハング問題修正
   - Int::SetupField内のMontgomery powers計算で無限ループが発生
   - IntMod.cppのSetupField関数を調査し、計算量削減または代替実装を検討
   - 一時的に簡易フィールド設定に変更して先に進むことも可能
2. 初期化0秒確認→10〜30秒計測→Key/s記録。
3. NEON RIPEMD-160を完成→再計測。
4. エンドモルフィズム修正→再計測。

## 完了済み項目（2024-12-29）
- ✅ STATIC_GTABLE: 動作確認済み（初期化0秒達成）
- ✅ Apple Silicon ビルドエラー修正
- ✅ NEON RIPEMD-160 関数名修正
- ✅ sha512.cpp x86アセンブリのポータブル化

## Makefileの主要フラグ
- Apple Silicon arm64: SSE除外、`clang++` + `-O3 -flto -arch arm64`
- `USE_LIBSECP256K1=1`: `-DUSE_LIBSECP256K1` 付与、`secp256k1_bridge.cpp`追加、`pkg-config`で`CFLAGS/LIBS`反映
- `STATIC_GTABLE=1`: `-DSTATIC_GTABLE` 付与（`k1_gtable.v1.inc`埋め込み経路）

## リスク/未完了項目
- STATIC_GTABLEが効いていない（最優先で修正）。
- NEON RIPEMD-160はWIP。
- `libsecp256k1`による連番鍵の最適化は未着手（自前Jacobian加算のみ）。
- 大型GTable（w幅拡大）の設計/容量検討未了。

## 連絡事項
- 以後の変更は本ドキュメント「直後にやるべき最小手順」に沿って進めてください。ビルド/実行コマンドは上記スニペットを利用可能です。


