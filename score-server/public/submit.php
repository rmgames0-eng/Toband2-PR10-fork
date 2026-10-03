<?php
declare(strict_types=1);
require_once dirname(__DIR__) . '/app.php';
header('Content-Type: application/json; charset=UTF-8');
header('Cache-Control: no-store');
try {
    if ($_SERVER['REQUEST_METHOD'] !== 'POST') { header('Allow: POST'); http_response_code(405); exit; }
    if ((int)($_SERVER['CONTENT_LENGTH'] ?? 0) > MAX_PAYLOAD) { http_response_code(413); exit; }
    if (isset($_SERVER['HTTP_ORIGIN']) && $_SERVER['HTTP_ORIGIN'] !== SCORE_BASE) { http_response_code(403); exit; }
    if (strtolower(explode(';',$_SERVER['CONTENT_TYPE'] ?? '')[0]) !== 'application/x-www-form-urlencoded') { http_response_code(415); exit; }
    $r = store_score($_POST, $_SERVER['REMOTE_ADDR']);
    http_response_code($r['created'] ? 201 : 200);
    echo json_encode(['ok'=>true,'id'=>$r['id'],'url'=>SCORE_BASE.'/score/?id='.$r['id']]);
} catch (InvalidArgumentException $e) { http_response_code(422); echo json_encode(['ok'=>false,'error'=>$e->getMessage()]);
} catch (OverflowException $e) { http_response_code(429); header('Retry-After: 3600'); echo json_encode(['ok'=>false,'error'=>$e->getMessage()]);
} catch (Throwable $e) { error_log('Score submission failed: '.$e->getMessage()); http_response_code(503); echo '{"ok":false,"error":"一時的に投稿できません。"}'; }
