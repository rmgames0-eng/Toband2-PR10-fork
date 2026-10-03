#!/usr/bin/python3
"""Consistent SQLite snapshot uploaded to the existing private backup bucket."""
import datetime
import json
import pathlib
import sqlite3
import tarfile
import tempfile
import urllib.request

BUCKET = 'toband2-wiki-backups-235830550102'
name = datetime.datetime.now(datetime.timezone.utc).strftime('score-%Y%m%dT%H%M%SZ.tar.gz')
with tempfile.TemporaryDirectory() as tmp:
    snapshot = pathlib.Path(tmp) / 'scores.sqlite'
    with sqlite3.connect('file:/var/lib/toband-score/scores.sqlite?mode=ro', uri=True) as source:
        with sqlite3.connect(snapshot) as dest:
            source.backup(dest)
    archive = pathlib.Path(tmp) / name
    with tarfile.open(archive, 'w:gz') as tar:
        tar.add(snapshot, arcname='scores.sqlite')
        tar.add('/var/www/toband-score', arcname='app', recursive=False)
        tar.add(pathlib.Path('/var/www/toband-score').resolve(), arcname='app-source')
    request = urllib.request.Request('http://metadata.google.internal/computeMetadata/v1/instance/service-accounts/default/token', headers={'Metadata-Flavor':'Google'})
    with urllib.request.urlopen(request, timeout=30) as response:
        token = json.load(response)['access_token']
    request = urllib.request.Request(
        f'https://storage.googleapis.com/upload/storage/v1/b/{BUCKET}/o?uploadType=media&name={name}&ifGenerationMatch=0',
        data=archive.read_bytes(), method='POST',
        headers={'Authorization':'Bearer '+token, 'Content-Type':'application/gzip'})
    with urllib.request.urlopen(request, timeout=120) as response:
        result = json.load(response)
    print('Backup uploaded:', result['name'], result['size'], 'bytes')
