# dropbox-sandbox

This Docker image provides a sandboxed environment for running a Dropbox daemon.

You should run it once with:

```
docker run -v /your-data-volume:/home/dropbox/Dropbox -ti rcarmo/dropbox-sandbox
```

To obtain the linking URL that will bind the container to your Dropbox account.

Sadly, Dropbox does not provide an easy way to either obtain that URL from a running instance nor to persist the binding data (it's stored inside one of the database files inside `.dropbox-dist`).
