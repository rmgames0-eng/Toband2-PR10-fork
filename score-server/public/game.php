<?php
require_once dirname(__DIR__).'/app.php';
page_start('TOband-R3');
?>
<h1>TOband-R3</h1>
<section>
<h2>●ゲーム本体</h2>
<a href="<?=RELEASE_URL?>">ダウンロード（Windows）</a><br>
<a href="<?=SOURCE_URL?>">開発・ソースコード・過去バージョン（GitHub）</a>
</section>
<section>
<h2>●スコアサーバー</h2>
<a href="/score/">新着順</a><br>
<a href="/score/?view=rank">スコア順</a><br>
<a href="/score/?view=win">勝利者</a><br>
<a href="/score/?view=stats">統計情報</a>
</section>
<section>
<h2>●リンク</h2>
<a href="/">TOband-R3 Wiki</a>
</section>
<section>
<h2>●開発</h2>
変愚蛮怒：変愚蛮怒開発チーム<br>
TOband2：TOband検討委員会<br>
TOband-R3：れんどる
</section>
<?php page_end(); ?>
