<?php
declare(strict_types=1);
const SCORE_BASE = 'https://toband-wiki.duckdns.org';
const RELEASE_URL = 'https://github.com/rmgames0-eng/Toband2-PR10-fork/releases/latest';
const SOURCE_URL = 'https://github.com/rmgames0-eng/Toband2-PR10-fork';
const MAX_PAYLOAD = 1572864;

function db(): PDO {
    static $db;
    if ($db) return $db;
    $dir = getenv('SCORE_DATA_DIR') ?: '/var/lib/toband-score';
    $db = new PDO('sqlite:' . $dir . '/scores.sqlite', null, null, [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION, PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC]);
    $db->exec('PRAGMA busy_timeout=5000; PRAGMA journal_mode=WAL;');
    $db->exec('CREATE TABLE IF NOT EXISTS scores (
        id INTEGER PRIMARY KEY, created TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        fingerprint TEXT NOT NULL UNIQUE, version TEXT NOT NULL, character TEXT NOT NULL,
        player TEXT NOT NULL, race TEXT NOT NULL, class TEXT NOT NULL, level INTEGER NOT NULL,
        class_level INTEGER NOT NULL, score INTEGER NOT NULL, turns INTEGER NOT NULL,
        depth INTEGER NOT NULL, outcome TEXT NOT NULL, cause TEXT NOT NULL, comment TEXT NOT NULL,
        dump TEXT NOT NULL);
        CREATE INDEX IF NOT EXISTS ranking ON scores(score DESC, id DESC);
        CREATE INDEX IF NOT EXISTS winners ON scores(outcome, score DESC);
        CREATE TABLE IF NOT EXISTS rate_limits (key TEXT PRIMARY KEY, started INTEGER NOT NULL, count INTEGER NOT NULL);');
    $columns = $db->query('PRAGMA table_info(scores)')->fetchAll(PDO::FETCH_COLUMN, 1);
    if (!in_array('party', $columns, true)) $db->exec("ALTER TABLE scores ADD COLUMN party TEXT NOT NULL DEFAULT '[]'");
    return $db;
}
function h(mixed $v): string { return htmlspecialchars((string)$v, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8'); }
function bad(string $message): never { throw new InvalidArgumentException($message); }
function text_field(array $data, string $key, int $max, bool $required = true): string {
    $value = $data[$key] ?? '';
    if (!is_string($value)) bad('投稿形式が正しくありません。');
    if (($data['encoding'] ?? '') === 'cp932') {
        if (!mb_check_encoding($value, 'SJIS-win')) bad('文字コードが正しくありません。');
        $value = mb_convert_encoding($value, 'UTF-8', 'SJIS-win');
    } elseif (($data['encoding'] ?? '') === 'euc-jp') {
        if (!mb_check_encoding($value, 'EUC-JP')) bad('文字コードが正しくありません。');
        $value = mb_convert_encoding($value, 'UTF-8', 'EUC-JP');
    } elseif (!mb_check_encoding($value, 'UTF-8')) bad('文字コードが正しくありません。');
    $value = trim(str_replace(["\r\n", "\r"], "\n", $value));
    if (preg_match('/[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]/', $value)) bad('使用できない文字が含まれています。');
    if (($required && $value === '') || mb_strlen($value) > $max) bad('項目の長さが正しくありません：' . $key);
    if ($key !== 'dump' && str_contains($value, "\n")) bad('改行はダンプ以外に使用できません。');
    return $value;
}
function integer_field(array $data, string $key, int $min, int $max): int {
    $v = $data[$key] ?? null;
    if (!is_string($v) || !preg_match('/^\d{1,12}$/D', $v) || (int)$v < $min || (int)$v > $max) bad('数値が正しくありません：' . $key);
    return (int)$v;
}
function validated_score(array $data): array {
    if (($data['protocol'] ?? '') !== '1' || !in_array($data['encoding'] ?? '', ['utf-8','cp932','euc-jp'], true)) bad('対応していない投稿形式です。');
    $s = [];
    foreach (['version'=>32,'character'=>80,'player'=>80,'race'=>80,'class'=>80,'cause'=>256,'comment'=>240,'dump'=>500000] as $k=>$max) $s[$k] = text_field($data, $k, $max, !in_array($k, ['player','comment']));
    foreach (['level'=>[1,50], 'class_level'=>[1,50], 'score'=>[0,2147483647], 'turns'=>[0,2147483647], 'depth'=>[0,9999]] as $k=>$bounds) $s[$k] = integer_field($data, $k, ...$bounds);
    $s['party'] = json_encode(validated_party($data), JSON_UNESCAPED_UNICODE | JSON_THROW_ON_ERROR);
    $s['outcome'] = $data['outcome'] ?? '';
    if (!in_array($s['outcome'], ['dead','winner'], true)) bad('結果が正しくありません。');
    if (!str_contains($s['dump'], '[TOband-R3 ') || strlen($s['dump']) < 100) bad('TOband-R3のキャラクターダンプが必要です。');
    // A retry may change the player/comment, but must not create another score.
    $s['fingerprint'] = hash('sha256', implode("\0", [$s['version'],$s['character'],$s['score'],$s['turns'],$s['dump']]));
    return $s;
}
function validated_party(array $data): array {
    if (!array_key_exists('party_count', $data)) return [];
    $count = integer_field($data, 'party_count', 1, 16);
    $members = []; $active = 0;
    for ($i = 0; $i < $count; ++$i) {
        $member = [];
        foreach (['name','race','class'] as $key) $member[$key] = text_field($data, "party_{$i}_{$key}", 80);
        foreach (['level'=>[1,50], 'class_level'=>[1,50], 'active'=>[0,1], 'dead'=>[0,1]] as $key=>$bounds)
            $member[$key] = integer_field($data, "party_{$i}_{$key}", ...$bounds);
        $active += $member['active'];
        $members[] = $member;
    }
    if ($active !== 1) bad('操作キャラクターの指定が正しくありません。');
    return $members;
}
function display_dump(string $dump): string {
    // Relocate the old party block without rewriting stored scores or fingerprints.
    if (preg_match('/^[ \t]*\[人物 1:[^\r\n]*\]\r?\n.*?^[ \t]*\[共有情報\][ \t]*\r?\n/msu', $dump, $m, PREG_OFFSET_CAPTURE) &&
        preg_match('/^[ \t]*\[(?:チェックサム|Check Sum):/mu', $dump)) {
        $block = preg_replace('/^[ \t]*\[共有情報\][ \t]*\r?\n/mu', '', $m[0][0]);
        $dump = substr_replace($dump, '', $m[0][1], strlen($m[0][0]));
        $dump = preg_replace_callback('/^[ \t]*\[(?:チェックサム|Check Sum):/mu',
            fn($match) => rtrim($block)."\n\n".$match[0], $dump, 1);
    }
    return preg_replace('/^[ \t]*\[共有情報\][ \t]*(?:\r?\n|$)/mu', '', $dump);
}
function adventure_ending(array $score): string {
    if ($score['outcome'] === 'winner') return '勝利の後引退';
    // The first status sentence belongs to the active character. Join wrapped lines.
    if (preg_match('/^[ \t]*…あなたは、?([^。]{1,1000})。/mu', $score['dump'] ?? '', $m)) {
        $ending = trim(preg_replace('/\r?\n[ \t]*/u', '', $m[1]));
        if (preg_match('/(?:殺された|石化された|武器に変化した)$/u', $ending)) return $ending;
    }
    // Old or non-Japanese dumps may lack a usable location; never invent one.
    $place = (int)$score['depth'] > 0 ? '場所不明の'.$score['depth'].'階' : '地上';
    return $place.'で'.$score['cause'].'に殺された';
}
function reserve_summaries(array $score): array {
    $members = json_decode($score['party'] ?? '[]', true) ?: [];
    $reserves = [];
    foreach ($members as $m) {
        if ($m['active']) continue;
        $reserves[] = $m['race'].'の'.$m['class'].'Lv'.$m['level'];
        if (count($reserves) === 2) break;
    }
    return array_pad($reserves, 2, '—');
}
function rate_limit(PDO $db, string $ip): void {
    $now = time();
    // Addresses are hashed with a private server key and expire after a day.
    $secret = file_get_contents((getenv('SCORE_DATA_DIR') ?: '/var/lib/toband-score') . '/rate.key');
    $key = hash_hmac('sha256', $ip, $secret);
    $db->prepare('DELETE FROM rate_limits WHERE started < ?')->execute([$now-86400]);
    $db->prepare('INSERT INTO rate_limits(key,started,count) VALUES(?,?,1) ON CONFLICT(key) DO UPDATE SET count=CASE WHEN started < ? THEN 1 ELSE count+1 END, started=CASE WHEN started < ? THEN excluded.started ELSE started END')->execute([$key,$now,$now-3600,$now-3600]);
    $q = $db->prepare('SELECT count FROM rate_limits WHERE key=?'); $q->execute([$key]);
    if ((int)$q->fetchColumn() > 10) throw new OverflowException('投稿回数の上限です。1時間後に再度お試しください。');
}
function store_score(array $data, string $ip): array {
    $db = db();
    $db->exec('BEGIN IMMEDIATE');
    try { rate_limit($db, $ip); $db->exec('COMMIT'); }
    catch (OverflowException $e) { $db->exec('COMMIT'); throw $e; }
    catch (Throwable $e) { $db->exec('ROLLBACK'); throw $e; }
    $s = validated_score($data);
    $keys = array_keys($s);
    $q = $db->prepare('INSERT INTO scores (' . implode(',', $keys) . ') VALUES (' . implode(',', array_fill(0,count($keys),'?')) . ') ON CONFLICT(fingerprint) DO NOTHING');
    $q->execute(array_values($s)); $new = $q->rowCount() > 0;
    $q = $db->prepare('SELECT id FROM scores WHERE fingerprint=?'); $q->execute([$s['fingerprint']]);
    return ['id'=>(int)$q->fetchColumn(), 'created'=>$new];
}
function page_start(string $title, string $selected=''): void {
    header('Content-Type: text/html; charset=UTF-8');
    header('Cache-Control: no-store');
    echo '<!doctype html><html lang="ja"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>'.h($title === 'TOband-R3' ? $title : $title.' — TOband-R3').'</title><link rel="stylesheet" href="/score/style.css?v=reserves2"><body><main>';
    if ($selected !== '') {
        echo '<nav aria-label="メイン"><a href="/game/">TOband-R3</a><a href="'.RELEASE_URL.'">ダウンロード</a><a href="/">Wiki</a><a href="'.SOURCE_URL.'">ソースコード</a></nav>'.($selected==='detail'?'':'<h1>'.h($title).'</h1>').'<nav class="tabs" aria-label="スコア">';
        foreach (['new'=>'新着','rank'=>'スコア順','win'=>'勝利者','stats'=>'統計'] as $key=>$label) echo '<a '.($key===$selected?'aria-current="page" ':'').'href="/score/?view='.$key.'">'.$label.'</a>';
        echo '</nav>';
    }
}
function page_end(): void { echo '</main></body></html>'; }
function result_name(string $outcome): string { return $outcome === 'winner' ? '勝利' : '死亡'; }
