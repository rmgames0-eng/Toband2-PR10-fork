<?php
// Run only with an explicitly isolated test database directory.
declare(strict_types=1);
if (PHP_SAPI !== 'cli' || !str_starts_with(getenv('SCORE_DATA_DIR') ?: '', '/tmp/toband-score-test-')) exit(2);
require __DIR__.'/app.php';
function check(bool $v): void { if (!$v) throw new RuntimeException('Assertion failed'); }
$raw=file_get_contents($argv[1]); parse_str($raw,$data);
$r=store_score($data,'test-client'); check($r['created']);
$r2=store_score($data,'test-client'); check(!$r2['created'] && $r['id']===$r2['id']);
$s=db()->query('SELECT * FROM scores')->fetch();
$party=json_decode($s['party'],true);
check(count($party)===3 && $party[0]['name']==='生存仲間');
check($party[0]['level']===23 && $party[0]['class_level']===21);
check($party[1]['active']===1 && $party[2]['dead']===1);
$reserves=reserve_summaries($s);
check($reserves===[$party[0]['race'].'の'.$party[0]['class'].'Lv23', $party[2]['race'].'の'.$party[2]['class'].'Lv12']);
check(reserve_summaries(['party'=>'[]'])===['—','—']);
$one=['party'=>json_encode([$party[1],$party[0]],JSON_UNESCAPED_UNICODE)];
check(reserve_summaries($one)===[$reserves[0],'—']);
$evil=$party[0];$evil['race']='<script>'; $evil['class']='&mage';
$escaped=h(reserve_summaries(['party'=>json_encode([$evil])])[0]);
check(str_contains($escaped,'&lt;script&gt;の&amp;mageLv23'));
$endingTest=['outcome'=>'dead','depth'=>39,'cause'=>'ダークプリースト','dump'=>"          …あなたは、死者の宮殿の39階でダークプリーストに殺された。"];
check(adventure_ending($endingTest)==='死者の宮殿の39階でダークプリーストに殺された');
$endingTest['dump']="  …あなたは、死者の宮殿の39階でダークプリーストに\r\n          殺された。";
check(adventure_ending($endingTest)==='死者の宮殿の39階でダークプリーストに殺された');
$endingTest['outcome']='winner';check(adventure_ending($endingTest)==='勝利の後引退');
$endingTest['outcome']='dead';$endingTest['dump']='';
check(adventure_ending($endingTest)==='場所不明の39階でダークプリーストに殺された');
$oldDump="先頭\n  [人物 1: 一 / 控え]\n詳細一\n  [人物 2: 二 / 操作中]\n詳細二\n  [共有情報]\n持ち物\n  [チェックサム: \"abc\"]\n";
$moved=display_dump($oldDump);
check(!str_contains($moved,'[共有情報]'));
check(strpos($moved,'持ち物')<strpos($moved,'[人物 1:'));
check(strpos($moved,'[人物 2:')<strpos($moved,'[チェックサム:'));
check(substr_count($moved,'詳細一')===1 && substr_count($moved,'詳細二')===1);
check(display_dump($moved)===$moved);
check(display_dump("ダンプのみ\n")==="ダンプのみ\n");
$legacy=$data; unset($legacy['party_count']);
check(validated_score($legacy)['party']==='[]');
foreach (['party_count'=>'17','party_0_level'=>'51','party_0_name'=>['bad'], 'party_0_dead'=>'2','party_0_active'=>'1'] as $key=>$value) {
    $bad=$data; $bad[$key]=$value;
    try { validated_score($bad); throw new RuntimeException('Invalid party accepted: '.$key); }
    catch (InvalidArgumentException $e) {}
}
check($s['character']==='送信確認用'); check($s['comment']==='<script>test & +</script>');
check(str_contains($s['dump'],'TOband-R3')); check(!str_contains(h($s['comment']),'<script>'));
foreach (['score'=>'-1','outcome'=>'cheater','level'=>'51','dump'=>'not a dump','character'=>['bad'],'encoding'=>'unknown'] as $key=>$value) {
    $bad=$data; $bad[$key]=$value;
    try { validated_score($bad); throw new RuntimeException('Invalid field accepted: '.$key); }
    catch (InvalidArgumentException $e) {}
}
$bad=$data; $bad['character']="\0";
try { validated_score($bad); throw new RuntimeException('NUL accepted'); } catch (InvalidArgumentException $e) {}
for ($i=0;$i<8;$i++) store_score($data,'test-client');
try { store_score($data,'test-client'); throw new RuntimeException('Rate limit failed'); } catch (OverflowException $e) {}
check((int)db()->query('SELECT count(*) FROM scores')->fetchColumn()===1);
// Rendering consumes the same stored CP932-origin data, escaped throughout.
$_SERVER['REQUEST_METHOD']='GET';
$_GET=['id'=>(string)$r['id']]; ob_start(); include __DIR__.'/public/index.php';
// index exits after rendering; its HTML is inspected by the test runner.
