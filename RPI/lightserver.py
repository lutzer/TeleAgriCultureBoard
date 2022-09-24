import threading
from time import sleep
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import idtable


class RGBColor:
    def __init__(self, r=0, g=0, b=0):
        self.r = r
        self.g = g
        self.b = b

    def __repr__(self):
        return 'color: ({0}, {1}, {2})'.format(self.r, self.g, self.b)

    def set_color(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b


class HandleRequests(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', '*')
        self.send_header('Content-type', 'application/json')
        self.end_headers()

    def do_OPTIONS(self):
        self._set_headers()

    def do_GET(self):
        path = '/whoami'
        if self.path == path:
            json_data = {'kit-id': self.server.kid}
            self._set_headers()
            self.wfile.write(json.dumps(json_data).encode('utf-8'))

    def do_PUT(self):
        path = '/color'
        if self.path == path:
            length = int(self.headers['Content-Length'])
            content = self.rfile.read(length)
            self._set_headers()
            self.server.color_lock.acquire()
            color_dict = json.loads(''.join(content.decode().split()))
            self.server.color.set_color(color_dict['red'], color_dict['green'], color_dict['blue'])
            self.server.color_lock.release()


class LightServer(threading.Thread):
    def __init__(self, host='', port=5555):
        threading.Thread.__init__(self)
        self.daemon = True

        id_finder = idtable.IDFinder()

        self.color = RGBColor()
        self.server = HTTPServer((host, port), HandleRequests)
        self.server.color = self.color
        self.server.kid = id_finder.get_id()
        self.server.color_lock = threading.Lock()

    def start_server(self):
        self.server.serve_forever()

    def stop_server(self):
        self.server.server_close()

    def run(self):
        self.start_server()

    def get_color(self):
        return self.color


if __name__ == '__main__':
    light_server = LightServer()
    light_server.start()
    is_running = True

    while is_running:
        try:
            print(light_server.get_color())
            sleep(1)
        except KeyboardInterrupt:
            is_running = False

    print('stopping lightserver')
    light_server.stop_server()
