from django.db import models
from django.contrib.auth.models import User

import datetime

# Create your models here.
class Pack(models.Model):
    id = models.AutoField(primary_key=True)
    title = models.CharField(max_length=255, unique=True)
    src = models.CharField(max_length=255)
    user = models.ForeignKey(User)
    rev = models.IntegerField(default=0)
    created = models.DateTimeField()
    updated = models.DateTimeField(default=datetime.datetime.min)
    summary = models.TextField()
    
#    def __str__(self):
#        return "<Pack " + self.title + ">"
#    def __repr__(self):
#        return self.__str__()