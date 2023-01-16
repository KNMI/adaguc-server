from flask_caching import Cache
cacher = Cache(config = {'CACHE_TYPE': 'SimpleCache'})

def init_cache(app):
    cacher.init_app(app)