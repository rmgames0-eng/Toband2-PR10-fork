#!/bin/bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq php8.2-sqlite3
stamp=$(date -u +%Y%m%dT%H%M%SZ)
release=/opt/toband-score/releases/$stamp
install -d -m 755 "$release"
tar -xzf /home/rmgam/toband-score.tar.gz -C "$release"
chown -R root:root "$release"
find "$release" -type d -exec chmod 755 {} +
find "$release" -type f -exec chmod 644 {} +
install -d -m 750 -o www-data -g www-data /var/lib/toband-score
if [ ! -f /var/lib/toband-score/rate.key ]; then
    openssl rand -hex 32 > /var/lib/toband-score/rate.key
    chown www-data:www-data /var/lib/toband-score/rate.key
    chmod 600 /var/lib/toband-score/rate.key
fi
find "$release" -name '*.php' -exec php -l {} \;
ln -s "$release" /var/www/toband-score-next
mv -Tf /var/www/toband-score-next /var/www/toband-score
cat > /etc/apache2/conf-available/toband-score.conf <<'EOF'
AliasMatch "^/game/?$" /var/www/toband-score/public/game.php
Alias /score /var/www/toband-score/public
<Directory /var/www/toband-score/public>
    Options -Indexes +FollowSymLinks
    AllowOverride None
    Require all granted
    DirectoryIndex index.php
    LimitRequestBody 1600000
    php_admin_flag file_uploads Off
    php_admin_value post_max_size 1600K
    php_admin_value max_execution_time 15
    php_admin_value memory_limit 64M
    php_admin_flag display_errors Off
</Directory>
EOF
a2enconf toband-score
apache2ctl configtest
sudo -u www-data php -r "require '/var/www/toband-score/app.php'; db();"
install -m 750 "$release/backup.py" /usr/local/sbin/toband-score-backup
printf '30 4 * * * root /usr/local/sbin/toband-score-backup >> /var/log/toband-score-backup.log 2>&1\n' > /etc/cron.d/toband-score-backup
chmod 644 /etc/cron.d/toband-score-backup
systemctl reload apache2
echo "Score server deployed: $release"
