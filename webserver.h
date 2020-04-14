String getContentType(String filename) {
    if (server.hasArg("download")) { return "application/octet-stream"; }
    else if (filename.endsWith(".htm")) { return "text/html"; }
    else if (filename.endsWith(".html")) { return "text/html"; }
    else if (filename.endsWith(".css")) { return "text/css"; }
    else if (filename.endsWith(".js")) { return "application/javascript"; }
    else if (filename.endsWith(".png")) { return "image/png"; }
    else if (filename.endsWith(".gif")) { return "image/gif"; }
    else if (filename.endsWith(".jpg")) { return "image/jpeg"; }
    else if (filename.endsWith(".ico")) { return "image/x-icon"; }
    else if (filename.endsWith(".xml")) { return "text/xml"; }
    else if (filename.endsWith(".pdf")) { return "application/x-pdf"; }
    else if (filename.endsWith(".zip")) { return "application/x-zip"; }
    else if (filename.endsWith(".gz")) { return "application/x-gzip"; }
    return "text/plain";
}

bool handleFileRead(String path) {
    if (path.endsWith("/")) {
        path += "index.htm";
    }
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (filesystem->exists(pathWithGz) || filesystem->exists(path)) {
        if (filesystem->exists(pathWithGz)) {
            path += ".gz";
        }
        File file = filesystem->open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void handleFileUpload() {
    if (server.uri() != "/edit") {
        return;
    }
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        fsUploadFile = filesystem->open(filename, "w");
        filename = String();
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile) {
            fsUploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {
            fsUploadFile.close();
        }
    }
}

void handleFileDelete() {
    if (server.args() == 0) {
        return server.send(500, "text/plain", "BAD ARGS");
    }
    String path = server.arg(0);
    if (path == "/") {
        return server.send(500, "text/plain", "BAD PATH");
    }
    if (!filesystem->exists(path)) {
        return server.send(404, "text/plain", "FileNotFound");
    }
    filesystem->remove(path);
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileCreate() {
    if (server.args() == 0) {
        return server.send(500, "text/plain", "BAD ARGS");
    }
    String path = server.arg(0);
    if (path == "/") {
        return server.send(500, "text/plain", "BAD PATH");
    }
    if (filesystem->exists(path)) {
        return server.send(500, "text/plain", "FILE EXISTS");
    }
    File file = filesystem->open(path, "w");
    if (file) {
        file.close();
    }
    else {
        return server.send(500, "text/plain", "CREATE FAILED");
    }
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileList() {
    if (!server.hasArg("dir")) {
        server.send(500, "text/plain", "BAD ARGS");
        return;
    }

    String path = server.arg("dir");
    Dir dir = filesystem->openDir(path);
    path = String();

    String output = "[";
    while (dir.next()) {
        File entry = dir.openFile("r");
        if (output != "[") {
            output += ',';
        }
        bool isDir = false;
        output += "{\"type\":\"";
        output += (isDir) ? "dir" : "file";
        output += "\",\"name\":\"";
        if (entry.name()[0] == '/') {
            output += &(entry.name()[1]);
        }
        else {
            output += entry.name();
        }
        output += "\"}";
        entry.close();
    }

    output += "]";
    server.send(200, "text/json", output);
}
