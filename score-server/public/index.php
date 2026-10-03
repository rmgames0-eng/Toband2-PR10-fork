<?php
declare(strict_types=1);
require_once dirname(__DIR__).'/app.php';
$view = is_string($_GET['view'] ?? null) ? $_GET['view'] : 'new';
if (!in_array($view,['new','rank','win','stats'],true)) { http_response_code(404); page_start('見つかりません'); echo '<p>ページが見つかりません。</p>'; page_end(); exit; }
if ($_SERVER['REQUEST_METHOD'] !== 'GET' && $_SERVER['REQUEST_METHOD'] !== 'HEAD') { header('Allow: GET, HEAD'); http_response_code(405); exit; }
$db = db();
if (isset($_GET['id'])) {
    $id = filter_var($_GET['id'],FILTER_VALIDATE_INT);
    $q = $db->prepare('SELECT * FROM scores WHERE id=?'); $q->execute([$id ?: 0]); $r=$q->fetch();
    if (!$r) { http_response_code(404); page_start('見つかりません'); echo '<h1>スコアが見つかりません。</h1>'; page_end(); exit; }
    $dump=display_dump($r['dump']);
    if (isset($_GET['download'])) {
        header('Content-Type: text/plain; charset=UTF-8'); header('X-Content-Type-Options: nosniff');
        header('Content-Disposition: attachment; filename="toband-score-'.$r['id'].'.txt"'); echo $dump; exit;
    }
    page_start('キャラクターダンプ','detail');
    echo '<pre class="dump">'.h($dump).'</pre>';
    page_end(); exit;
}
$titles=['new'=>'新着スコア','rank'=>'スコアランキング','win'=>'勝利者','stats'=>'統計情報']; page_start($titles[$view],$view);
if ($view==='stats') {
    $tot=$db->query("SELECT COUNT(*) count,COALESCE(SUM(outcome='winner'),0) wins,COALESCE(MAX(score),0) best FROM scores")->fetch();
    echo '<div class="stats"><div><span>冒険の記録</span><strong>'.number_format($tot['count']).'</strong></div><div><span>勝利</span><strong>'.number_format($tot['wins']).'</strong></div><div><span>最高スコア</span><strong>'.number_format($tot['best']).'</strong></div></div>';
    foreach (['class'=>'職業','race'=>'種族'] as $col=>$label) {
        echo '<h2>'.$label.'別</h2><div class="table-wrap"><table><thead><tr><th>'.$label.'</th><th>記録</th><th>勝利</th><th>最高スコア</th></tr></thead><tbody>';
        foreach ($db->query("SELECT $col label,COUNT(*) count,SUM(outcome='winner') wins,MAX(score) best FROM scores GROUP BY $col ORDER BY count DESC, label") as $r) echo '<tr><td>'.h($r['label']).'</td><td>'.$r['count'].'</td><td>'.$r['wins'].'</td><td>'.number_format($r['best']).'</td></tr>';
        echo '</tbody></table></div>';
    }
} else {
    $page=max(1,min(100000,(int)($_GET['page']??1))); $where=$view==='win'?" WHERE outcome='winner'":'';
    $class=is_string($_GET['class']??null)?mb_substr($_GET['class'],0,80):'';
    $params=[];
    if ($class!=='') { $where.=($where?' AND ':' WHERE ').'class=?'; $params[]=$class; }
    $q=$db->prepare('SELECT COUNT(*) FROM scores'.$where); $q->execute($params); $count=(int)$q->fetchColumn(); $pages=max(1,(int)ceil($count/25)); $page=min($page,$pages); $offset=($page-1)*25;
    echo '<form method="get" class="filter"><input type="hidden" name="view" value="'.h($view).'"><label>職業 <select name="class"><option value="">すべて</option>';
    foreach ($db->query('SELECT DISTINCT class FROM scores ORDER BY class') as $r) echo '<option '.($class===$r['class']?'selected ':'').'value="'.h($r['class']).'">'.h($r['class']).'</option>';
    echo '</select></label><button type="submit">絞り込む</button><span>'.$count.'件</span></form>';
    $q=$db->prepare('SELECT id,created,character,player,race,class,level,class_level,score,depth,outcome,cause,comment,party,dump FROM scores'.$where.' ORDER BY '.($view==='new'?'id DESC':'score DESC,id DESC').' LIMIT 25 OFFSET '.$offset); $q->execute($params);
    if (!$count) echo '<p class="empty">まだスコアは登録されていません。</p>';
    else {
        echo '<div class="table-wrap"><table class="scores"><thead><tr><th>#</th><th>スコア</th><th>キャラクター</th><th>種族</th><th>職業</th><th>レベル</th><th>控え１</th><th>控え２</th><th>冒険の結末</th><th>登録日</th></tr></thead><tbody>';
        foreach ($q as $i=>$r) {
            $reserves=reserve_summaries($r);
            echo '<tr><td class="muted">'.($offset+$i+1).'</td><td class="number">'.number_format($r['score']).'</td><td><a class="name" href="?id='.$r['id'].'">'.h($r['character']).'</a></td><td>'.h($r['race']).'</td><td>'.h($r['class']).'</td><td>'.$r['level'].'</td><td class="reserve">'.h($reserves[0]).'</td><td class="reserve">'.h($reserves[1]).'</td><td>'.h(adventure_ending($r)).'</td><td>'.h(substr($r['created'],0,10)).'</td></tr>';
        }
        echo '</tbody></table></div>';
    }
    if ($pages>1) { echo '<nav class="pagination">'; foreach ([$page-1=>'← 前へ',$page+1=>'次へ →'] as $p=>$label) if ($p>=1&&$p<=$pages) echo '<a href="?'.h(http_build_query(['view'=>$view,'class'=>$class,'page'=>$p])).'">'.$label.'</a>'; echo '<span>'.$page.' / '.$pages.'</span></nav>'; }
}
page_end();
