var system = require('system'),
    page = require('webpage').create(),
    Routes = require('./Routes.js'),
    app = new Routes();

page.viewportSize = page.clipRect = {width: 1280, height: 1024};

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
    if(!req.post.url){
        res.send("No URL received", 503);
    }
    if(req.post.width && req.post.height){
        page.viewportSize = page.clipRect = {width: req.post.width, height: req.post.height};
    }
    page.customHeaders = {'Referer': req.post.url};
    page.settings.javascriptEnabled = true;
    page.settings.localToRemoteUrlAccessEnabled = true;
    page.settings.XSSAuditingEnabled = true;
    page.settings.webSecurityEnabled = false;
    page.settings.userAgent = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/27.0.1453.116 Safari/537.36';
    page.open(req.post.url,function(status){
        if(status !== "success"){
            res.send(status);
        }
        else {
            page.evaluate(function() {
                document.body.bgColor = 'white';
            });
            setTimeout(function() {
                var pic = page.renderBase64("png");
                res.headers = {
                    "Content-Type": "image/png",
                    "Content-Transfer-Encoding": "base64"
                }
                res.send(pic); 
            }, req.post.timeout || 1000);
        }
    });    
});

app.listen(system.args[1] || 8000);

console.log('Listening on port ' + (system.args[1] || 8000));
