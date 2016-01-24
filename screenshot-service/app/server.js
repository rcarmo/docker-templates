var system = require('system'),
    page = require('webpage').create(),
    Routes = require('./Routes.js'),
    fs = require('fs'),
    staticRoot = '/tmp/screenshots',
    app = new Routes();

page.viewportSize = page.clipRect = {width: 1280, height: 1024};

fs.makeDirectory(staticRoot);

app.use(Routes.static(staticRoot));

app.get('/',function(req,res) {
    res.send("", 200);
});

app.use(function(req,res,next){
    if(req.post.width && req.post.height){
        if(isNaN(parseInt(req.post.width)) && isNaN(parseInt(req.post.height))){
            req.post.width = Math.abs(Math.floor(req.post.width));
            req.post.height = Math.abs(Math.floor(req.post.height));    
        }
        else{
            req.post.width = null;
            req.post.height = null;
        }
    }
    next();
});

app.post('/',function(req, res) {
    if(!req.post.key){
        res.send("No key received", 403);
    }
    if(req.post.key != system.env["API_KEY"]){
        res.send("Invalid key received", 403);
    }
    if(!req.post.url){
        res.send("No URL received", 503);
    }
    if(req.post.width && req.post.height){
        page.viewportSize = page.clipRect = {width: req.post.width, height: req.post.height};
    }
    guid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = crypto.getRandomValues(new Uint8Array(1))[0]%16|0, v = c == 'x' ? r : (r&0x3|0x8);
        return v.toString(16);
    });
    page.customHeaders = {'Referer': req.post.url};
    page.settings.javascriptEnabled = true;
    page.settings.localToRemoteUrlAccessEnabled = true;
    page.settings.XSSAuditingEnabled = true;
    page.settings.webSecurityEnabled = false;
    page.settings.userAgent = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/27.0.1453.116 Safari/537.36';
    var expiry = new Date().getTime() - 3600000;
    Array.prototype.map(function(c,i,a){
        var file = staticRoot + '/' + c;
        if(fs.isFile(file) && (fs.lastModified(file).getTime() < expiry)) {
            fs.remove(file);
        }
    }, fs.list(staticRoot))
    page.open(req.post.url,function(status){
        if(status !== "success"){
            res.send(status, 500);
        }
        else {
            if(!req.post.transparent){
                page.evaluate(function() {
                    document.body.bgColor = 'white';
                });
            }
            setTimeout(function() {
                page.render(staticRoot + '/' + guid + '.jpg', {format: "jpeg", quality: "100"});
            }, req.post.timeout || 1000);
            res.send(guid + '.jpg', 200);
        }
    });    
});

app.listen(system.args[1] || 8000);

console.log('Listening on port ' + (system.args[1] || 8000));
