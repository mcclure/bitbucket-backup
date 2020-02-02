import Image
import glob
import os
import sys
import xml.dom.minidom

# Adapted from http://www.retroaffect.com/blog/159/Image_Atlas_Packer/

##    Settings    ##

#Directory to load images from
imageDirectory = './source_image/'

#Directory to save the atlas images and the atlas dictionary
saveDirectory = './output_image/'

#Also, the variable atlasSize dictates the size of each exported atlas

##  End Settings  ##


#Create the Dictionary
doc = xml.dom.minidom.Document()
textureAtlas = doc.createElement("list")
textureAtlas.setAttribute("name", "textureAtlas")
doc.appendChild(textureAtlas)

##### Image Packer #####

class PackAtlas(object):

    #Create an image that can fit smaller images within it
    def __init__(self, area):
        #Assume the two elements within the tupple are width and height, and origin is (0,0)
        if len(area) == 2:
            area = (0,0,area[0],area[1])
        self.area = area

    def __repr__(self):
        return "<%s %s>" % (self.__class__.__name__, str(self.area))

    def get_width(self):
        return self.area[2] - self.area[0]
    width = property(fget=get_width)

    def get_height(self):
        return self.area[3] - self.area[1]
    height = property(fget=get_height)

    def insert(self, area):
        if hasattr(self, 'child'):
            a = self.child[0].insert(area)
            if a is None: return self.child[1].insert(area)
            return a

        area = PackAtlas(area)
        
        if area.width <= self.width and area.height <= self.height:
            self.child = [None,None]
            self.child[0] = PackAtlas((self.area[0]+area.width, self.area[1], self.area[2], self.area[1] + area.height))
            self.child[1] = PackAtlas((self.area[0], self.area[1]+area.height, self.area[2], self.area[3]))
            return PackAtlas((self.area[0], self.area[1], self.area[0]+area.width, self.area[1]+area.height))
    
##### XML Generation #####

# Original / Retro Affect format
class RetroAffectXmlDump:
    def render(self, doc, textureAtlas, left, right, top, bottom, filename, aDirName, atlasNumber):        
        #Dictionary Entry
        table = doc.createElement("table")
        textureAtlas.appendChild(table)

        #Name
        paragraph1 = doc.createElement("string")
        paragraph1.setAttribute("name", "name")
        table.appendChild(paragraph1)

        ptext = doc.createTextNode( filename )
        paragraph1.appendChild(ptext)

        #Atlas Name
        paragraph1 = doc.createElement("string")
        paragraph1.setAttribute("name", "atlas" )
        table.appendChild(paragraph1)

        ptext = doc.createTextNode( "." + saveDirectory[33:] + aDirName + "-" + str(atlasNumber) + ".png" )
        paragraph1.appendChild(ptext)

        #Image location in the atlas
        
        #Left
        paragraph1 = doc.createElement("real")
        paragraph1.setAttribute("name", "left")
        table.appendChild(paragraph1)

        ptext = doc.createTextNode( left )
        paragraph1.appendChild(ptext)

        #Top
        paragraph1 = doc.createElement("real")
        paragraph1.setAttribute("name", "top")
        table.appendChild(paragraph1)

        ptext = doc.createTextNode( top )
        paragraph1.appendChild(ptext)

        #Right
        paragraph1 = doc.createElement("real")
        paragraph1.setAttribute("name", "right")
        table.appendChild(paragraph1)

        ptext = doc.createTextNode( right )
        paragraph1.appendChild(ptext)

        #Bottom
        paragraph1 = doc.createElement("real")
        paragraph1.setAttribute("name", "bottom")
        table.appendChild(paragraph1)

        ptext = doc.createTextNode( bottom )
        paragraph1.appendChild(ptext)

# Jumpcore format
class JumpcoreXmlDump:
    def __init__(self):
        self.elems = {}
        
    def render(self, doc, textureAtlas, left, right, top, bottom, filename, aDirName, atlasNumber):        
        atlasname = aDirName + "-" + str(atlasNumber) + ".png"
        
        if (atlasname in self.elems):
            elem = self.elems[atlasname]
        else:
            elem = doc.createElement("atlas")
            self.elems[atlasname] = elem
            elem.setAttribute("filename", atlasname)
            elem.setAttribute("basename", aDirName) # Feels unnecessary.
            elem.setAttribute("index", str(atlasNumber))
            textureAtlas.appendChild(elem)
    
        table = doc.createElement("image")
        elem.appendChild(table)
        
        table.setAttribute("name", filename.split("/")[-1])
        table.setAttribute("left", left)
        table.setAttribute("right", right)
        table.setAttribute("top", top)
        table.setAttribute("bottom", bottom)
    
##### Main #####

def packImages( aDir, aDirName ):
    atlasSize = 128, 128 #size of each atlas
    format = 'RGBA' #image format
    names = glob.glob(aDir + "/*.png") #file type of the packable images

    atlasNumber = 1 #saving convention
    atlasList = []
    
    xmldump = JumpcoreXmlDump()

    #create a list of image objects, sorted by size
    if names:        
        images = sorted([(i.size[1], name, i) for name,i in ((x,Image.open(x)) for x in names)], reverse=True)
        #Create the Atlas
        tree = PackAtlas(atlasSize)
        image = Image.new(format, atlasSize)
        atlasList.append( (tree,image) )
    else:
        images = []
        
    #insert each image into the PackAtlas area and create the dictionary entries
    for area, name, img in images:
        
        #check if & where the current texture fits
        #anywhere where i try to insert, go through a for loop to see if it inserts there
        inserted = False
        insertedAtlas = 0 # XML render will need to know which atlas we inserted in
        iterAtlas = 0     # Used to track atlasList indices during foreach
        for entry in atlasList:
            uv = entry[0].insert(img.size)
            iterAtlas += 1
            if uv:
                entry[1].paste(img, uv.area)
                inserted = True
                insertedAtlas = iterAtlas
                break
        
        if not inserted:
            tree = PackAtlas(atlasSize)
            uv = tree.insert(img.size)
            image = Image.new(format, atlasSize)
            atlasList.append( (tree,image) )
            image.paste(img, uv.area)
            insertedAtlas = iterAtlas + 1 # i.e. last index in atlasList plus one
        
        #location data for the dictionary     
        left = uv.area[0]
        top = uv.area[1]
        right = uv.area[0] + img.size[0]
        bottom = uv.area[1] + img.size[1]
        
        left = str(float(left)/atlasSize[0])
        top = str(float(top)/atlasSize[1])        
        right = str(float(right)/atlasSize[0])
        bottom = str(float(bottom)/atlasSize[1])
        
        filename = img.filename[:-4].replace("\\","/")    
        
        xmldump.render(doc, textureAtlas, left, right, top, bottom, filename, aDirName, insertedAtlas)

    atlasNumber = 1
    for i in atlasList:
        i[1].save( saveDirectory + aDirName + "-" + str(atlasNumber) + ".png")
        atlasNumber = atlasNumber + 1

    # loop through every directory
    for f in os.listdir( imageDirectory ):
        if ( os.path.isdir(aDir + f + "/") ):
            packImages( aDir + f + "/", aDirName + f )


if __name__ == "__main__":

    packImages( imageDirectory, "" )
    #save the dictionary
    file_object = open(saveDirectory + "atlasDictionary.xml", "w")
    doc.writexml(file_object, indent="\n", addindent="    ")
    file_object.close()
    
