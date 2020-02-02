from django.conf.urls.defaults import *

# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()

urlpatterns = patterns('',

    (r'^$', 'levels.app.views.index'),
    (r'^upload$', 'levels.app.views.upload'),
    (r'^browse$', 'levels.app.views.browse'),
    (r'^login$', 'levels.app.views.login'),
    (r'^tos$', 'levels.app.views.tos'),
    # Example:
    # (r'^levels/', include('levels.foo.urls')),

	# STATIC SERVES:
	(r'^landing/(?P<path>.*)$', 'django.views.static.serve', {'document_root': 'landing'}),

    # Uncomment the admin/doc line below and add 'django.contrib.admindocs' 
    # to INSTALLED_APPS to enable admin documentation:
    # (r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    # (r'^admin/', include(admin.site.urls)),
)
