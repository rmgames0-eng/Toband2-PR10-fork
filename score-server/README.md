# TOband-R3 score server

Public game page: https://toband-wiki.duckdns.org/game/

Scores: https://toband-wiki.duckdns.org/score/

PHP 8.2 + PDO SQLite, hosted alongside PukiWiki on the existing GCP
`toband2-wiki` VM in `us-west1-b`. No extra VM, address or domain.

## Deployment

Archive this directory's contents as `toband-score.tar.gz`. Copy the archive
and `deploy.sh` to `/home/rmgam/` using `gcloud compute scp` with IAP, then run
`sudo bash /home/rmgam/deploy.sh`. The script keeps dated application releases
in `/opt/toband-score/releases`, links `/var/www/toband-score` to the current
release, installs the SQLite module, and enables the two Apache aliases.
Existing Wiki files, TLS certificates and redirects are untouched.

The SQLite database and rate-limit key live in `/var/lib/toband-score`, outside
the public tree. Only `www-data` and root can access this directory.
Production deployment must preserve this data directory. Do not serve the
repository root as a document root; only `public/` is public.

Daily 04:30 VM-local-time backups use SQLite's snapshot API and the existing
private GCS bucket `toband2-wiki-backups-235830550102` (14-day lifecycle).
Job: `/etc/cron.d/toband-score-backup`; log: `/var/log/toband-score-backup.log`.
`sudo /usr/local/sbin/toband-score-backup` runs an immediate backup.

To restore a database, temporarily disable score POSTs, stop Apache, retain a
copy of the existing database/WAL/SHM, restore the snapshot as `scores.sqlite`
with owner `www-data`, remove only the obsolete WAL/SHM for that database,
then restart Apache and check both pages and submission. Never copy a live
SQLite main file without its WAL; use SQLite's backup API.

## Submission

`POST /score/submit.php`, `application/x-www-form-urlencoded`, HTTPS only.
Protocol `1`: `encoding` is `cp932`, `euc-jp` or `utf-8`; text fields are
`version`, `character`, `player`, `race`, `class`, `cause`, `comment`, `dump`;
integer fields are `level`, `class_level`, `score`, `turns`, `depth`;
`outcome` is `dead` or `winner`. The last-words field (`comment`) may be empty. `player` is accepted only for older clients and is no longer displayed or sent by the current game.
Responses: 201 new, 200 duplicate, 422 malformed, 429 rate limit, 503 unavailable.
Only a normal final death / retired winner accepted by the game's existing
`check_score()` is offered submission. Individual companion deaths and ordinary
saves do not post. No save files or credentials are uploaded.

The game asks whether to send the score to the score server and accepts optional last words (遺言). WinHTTP validates the
server certificate, has bounded timeouts and refuses redirects. No submission file is written and there is no manual upload/retry page.
Identical payloads are deduplicated. Non-Windows builds do not send automatically.

All user content is escaped on output and SQL uses prepared parameters.
Payload limit is 1.5 MiB; submissions are limited to 10 per IP per hour. Rate
keys are HMACs of addresses and expire after a day. Apache's existing access
logs still record request IPs. Scores are client-reported, not cryptographically
verified; use administrative review for fraudulent posts. Moderation is via
local SQLite over IAP SSH, not an unauthenticated web deletion endpoint.
Always snapshot before deleting a record and identify it by ID and fingerprint.

## Tests

`tests/test_score_submission.py` creates a genuine game dump and tests its
CP932 form encoding without sending anything or creating automatic submission files.
Its explicit `--live` switch tests WinHTTP against production and inserts a
clearly marked disposable record; remove exactly that record after checking it.

`test.php` validates a client payload, rejection cases, duplicates, rate limits,
and escaped rendering against an isolated `SCORE_DATA_DIR=/tmp/toband-score-test-*`.
Copy app files there, create a random `rate.key`, and pass a generated `.tbs`.
It never defaults to the live database.

The public download link points to the existing latest GitHub release.
Deploying this server does not publish a game release or push local game changes.

Optional party fields (protocol 1, backward compatible): `party_count` (1..16),
then `party_N_name`, `party_N_race`, `party_N_class`, `party_N_level`,
`party_N_class_level`, `party_N_active` and `party_N_dead` for N=0..count-1.
Levels are 1..50, flags 0/1, exactly one member is active. All members,
including the active character and dead companions, are included. Names use
`encoding` like other fields. Stored as UTF-8 JSON in the `party` column;
old clients remain accepted and records without party data show 未登録.
