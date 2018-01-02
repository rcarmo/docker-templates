# dropbox-sandbox

This Docker image provides a sandboxed environment for running a Dropbox daemon.

You should run it once with:

```
docker run -v /your-data-volume:/Dropbox -ti rcarmo/dropbox
```

To obtain the linking URL that will bind the container to your Dropbox account.

Sadly, Dropbox does not provide an easy way to either obtain that URL from a running instance nor to persist the binding data (it's stored inside one of the database files inside `.dropbox-dist`).

A nicer (and more thoroughly tested) alternative is [janeczku/dropbox](https://github.com/janeczku/docker-dropbox), but that is based off Debian 8 instead of Ubuntu.
