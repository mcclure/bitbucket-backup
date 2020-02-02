from django.http import HttpResponse, HttpResponseRedirect
from django.template import RequestContext, loader
from django import forms
from levels import local_settings
from django.contrib.auth import login as ulogin, authenticate
from django.contrib.auth.models import User

from models import *

# Create your views here.

def index(request):
    return HttpResponse("What are you doing go away");
    
class UploadFileForm(forms.Form):
    file  = forms.FileField()
    filename = forms.CharField(max_length=100)
    summary = forms.CharField(label='Summary', required=False)
    
class LoginForm(forms.Form):
    username = forms.CharField(label='Username')
    password = forms.CharField(label='Password', widget=forms.widgets.PasswordInput())
    email = forms.CharField(label='Email', required=False)
    
def handle_uploaded_file(f, filename):
    if not filename or filename.find("/") >= 0 or filename.find(".") == 0:
        filename = "illegal.tgz"; # This is nonsense
        
    destination = open(local_settings.OUR_DIR + 'landing/' + filename, 'wb+')
    for chunk in f.chunks():    
        destination.write(chunk)
    destination.close()
    
def upload(request):
    context = {}
    if request.method == 'POST':    
        obj = None
        if request.user.is_anonymous():
            context['message'] = "I'm sorry, somehow you aren't logged in. Please close and reopen Jumpman."
        else:
            form = UploadFileForm(request.POST, request.FILES)                
            
            if form.is_valid():
                obj = Pack.objects.filter(title__exact=request.POST['filename'])
            
                if not obj:
                    context['message'] = "Uploaded new level pack."
                    obj = Pack()
                    obj.title = request.POST['filename']
                    obj.created = datetime.datetime.now()
                    obj.user = request.user;
                    obj.save()
                else:
                    obj = obj[0]
                    if obj.user.id == request.user.id:
                        context['message'] = "Updated your level pack."
                    else:
                        context['message'] = "There is already a level pack by that name. Rename and try again."
                        obj = None

            else:
                context['message'] = form.errors
                
        if obj:
            obj.rev += 1
            obj.updated = datetime.datetime.now()
            obj.summary = request.POST['summary']
            obj.src = str(obj.id) + "_" + str(obj.rev) + ".tgz"
            handle_uploaded_file(request.FILES['file'], obj.src)
            obj.save()
    else:
        context['form'] = UploadFileForm()
    t = loader.get_template('levels/form.html')
    c = RequestContext(request, context)    
    return HttpResponse(t.render(c))
    
def browse(request):
    files = Pack.objects.all().order_by('-updated')
    context = {'files' : files, 'user' : request.user}
    t = loader.get_template('levels/browse.html')
    c = RequestContext(request, context)    
    return HttpResponse(t.render(c))

def tos(request):
    context = {}
    t = loader.get_template('levels/tos.html')
    c = RequestContext(request, context)    
    return HttpResponse(t.render(c))

def login(request):
    context = {}
    success = False
    if request.method == 'POST':    
        form = LoginForm(request.POST)
#        context['form'] = form

        if form.is_valid():
            user = authenticate(username=request.POST['username'],
                        password=request.POST['password'])

            if user is not None:
                if user.is_active:
                    ulogin(request, user)
                    context['message'] = "Welcome"
                    success = True
                else:
                    context['message'] = "This user account has been disabled."
            else:
                try:
                    user = User.objects.get(username=request.POST['username'])
                    context['message'] = "There is already a user by that name, but that is not the right password. Try again?"
                except User.DoesNotExist:
                    # Create a new user. Note that we can set password
                    # to anything, because it won't be checked; the password
                    # from settings.py will.
                    user = User.objects.create_user(request.POST['username'], request.POST['email'], request.POST['password'])
                    context['message'] = "Welcome new user"
                    success = True
        else:
            context['message'] = "Sorry, something is broken."
    else:
        context['form'] = LoginForm()
    t = loader.get_template('levels/form.html')
    c = RequestContext(request, context)    
    return HttpResponse(content=t.render(c), status=200 if success else 401)