# Python script to generate documentation
# Put documentation in doc folder.

import sys
import os

src_folder = sys.argv[1] + "/" + sys.argv[3] # Contains the file to generate the doc from.
doc_folder = sys.argv[1] + "/doc"
project_name = sys.argv[2]


# print(src_folder,"->",doc_folder)

# doc_data contains the format: {file: path, content: content_of_doc,declaration: declaration}
# doc_data example for format. The rendered html looks like this:
'''
<div>
	<!-- postDocContent inside an h3 tag -->
	<h3>int exec(const char * cmd,std::string * result,FILE * outputStream)</h3>

	<!-- The docContent is plain text, html can be easily included with it. -->
	Execute the command cmd, write the result to the result string and to an outputStream if one is provided.
	<!-- Things between @code and @endcode are converted to this -->
	<pre>
		<code class="cpp">#include "exec.h"</code>
		<code class="cpp"></code>
		<code class="cpp">int main(){</code>
		<code class="cpp">	std::string result;</code>
		<code class="cpp">	int returnCode = exec("echo a",&result,stdout);</code>
		<code class="cpp">	return 0;</code>
		<code class="cpp">}</code>
	</pre>
	<!-- filepath is put at the end like this -->
	<em>exec.h</em>
	<!-- search tags are here -->
	<span class='hidden tag'>Some tags or stuff</span>
</div>
'''

doc_data = []

def genDocData(docData,docContent,postDocContent,filepath,isMain = False):
	# convert everything to proper html and packs it into a neat dict.
	generated_html = "<div>"
	if isMain:
		generated_html += "<h2>" + postDocContent + "</h2>"
	else:
		generated_html += "<h3>" + postDocContent + "</h3>"
	# parse code tags inside docContent
	inCode = False
	currentStr = ""
	i = 0

	while i < len(docContent):
		if not inCode and docContent.startswith("@code",i):
			if currentStr != "":
				generated_html += currentStr
				currentStr = ""
			inCode = True
			generated_html += "<pre>"
			i += len("@code") - 1
		elif inCode and docContent.startswith("@endcode",i):
			inCode = False
			if currentStr != "":
				generated_html += "<code class='cpp'>" + currentStr + "</code>"
				currentStr = ""
			generated_html += "</pre>"
			i += len("@endcode") - 1
		elif inCode and docContent[i] == '\n':
			# flush str.
			generated_html += "<code class='cpp'>" + currentStr + "</code>"
			currentStr = ""
		else:
			currentStr += docContent[i]
		i += 1

	if inCode: # close pre tag so that the html stays valid even is the @code is never closed
		if currentStr != "":
			generated_html += "<code class='cpp'>" + currentStr + "</code>"
			currentStr = ""
		generated_html += "</pre>"
	else:
		if currentStr != "":
			generated_html += currentStr

	if not isMain:
		generated_html += "<em>" + filepath + "</em>"

	generated_html += "</div>"
	# if main, add it at the beginning.
	if isMain:
		docData.insert(0,generated_html)
	else:
		docData.append(generated_html)


for root, subdirs, files in os.walk(src_folder):
	for file in files:
		full_path = root + "/" + file
		contains_doc = False

		with open(full_path,'r') as cfile:
			file_content = cfile.read()
			i = 0
			inDoc = False
			inCode = False
			inPostDoc = False

			inComment = False
			inLineComment = False

			docContent = ""
			postDocContent = ""

			while i < len(file_content):
				if not inDoc and file_content.startswith("/**",i):
					inDoc = True
					i += len("/**") - 1
				elif inDoc and file_content.startswith("*/",i):
					inDoc = False
					docContent = docContent.strip()
					if docContent.lower().startswith("overview"):
						docContent = docContent[len("overview"):]
						genDocData(doc_data,docContent,"Overview","",True)
						docContent = ""
						# inPostDoc == False here
					else:
						inPostDoc = True
					i += len("*/") - 1
				elif inDoc:
					docContent += file_content[i]
				elif inPostDoc: # try to find the declaration the documentation is refering to.
					if (not inComment and not inLineComment) and (file_content[i] == '{' or file_content[i] == ';'):
						# OK, full doc can be processed.
						postDocContent = postDocContent.strip()
						genDocData(doc_data,docContent,postDocContent,full_path)
						docContent = ""
						postDocContent = ""
						inPostDoc = False
					elif not inComment and file_content.startswith("/*",i):
						inComment = True
						i += len("/*") - 1
					elif not inLineComment and file_content.startswith("//",i):
						inLineComment = True
						i += len("//") - 1

					elif inComment and file_content.startswith("*/",i):
						inComment = False
						i += len("*/") - 1
					elif inLineComment and file_content[i] == '\n':
						inLineComment = False
						# i += len("\n") - 1
					elif file_content[i] != '\n' and not inComment and not inLineComment:
						postDocContent += file_content[i]

				i += 1

full_doc = "<!DOCTYPE html><html><head>" # contains the full html doc
full_doc += "<title>Documentation - "+project_name+"</title>"
full_doc += """
<style>
html,body{
	margin:0;
	padding:0;
	display:flex;
	flex-direction:column;
	font-family:Helvetica;
}
nav{
	width:100%;
	background-color:#eee;
	padding:10px;
	display: flex;
	align-items:center;
}
h1{
	margin:0;
	padding:0 1em;
}
input{
	padding:5px;
	font-size:1em;
	outline:none;
}
section{
	display:flex;
	flex-direction:column;
}
section > div{
	margin:10px;
	padding:10px;
	border-bottom:1px solid black;
	margin-top:0;
	padding-top:0;
}
h3{
	font-family:monospace;
	margin-top:0;
}
pre{
	margin:0.5em;
	white-space:nowrap;
}
pre code.cpp {
	display:block;
	white-space:pre;
	font-family: monospace;
	padding:.3em;
	tab-size: 4;
}
.main{
	padding-bottom:20px;
}
em{
	display: block;
	padding:5px;
}
</style>
</head>
<body>
"""

full_doc += "<nav><input type='text' autocomplete='off' id='s' placeholder='Search'><h1>Documentation - " + project_name + "</h1></nav>"
full_doc += "<section>"

for data in doc_data:
	full_doc += data

full_doc += "</section></body>"

# search script
full_doc += """ 
<script>
	let els = document.querySelectorAll('section>div');
	document.getElementById("s").addEventListener('keydown',function(){
		setTimeout(() => {
			let query = this.value;
			if(query == ""){
				for(let i = 0;i < els.length;i++){
					els[i].style.display = '';
				}
			}else{
				for(let i = 0;i < els.length;i++){
					if(els[i].innerText.toLowerCase().indexOf(query) == -1){
						els[i].style.display = 'none';
					}else{
						els[i].style.display = '';
					}
				}
			}
		},1);
	});
</script>
"""

full_doc += """
<link rel="stylesheet"
  href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/10.0.2/styles/default.min.css">
<script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/10.0.2/highlight.min.js"></script>
<script>
// this fails if no internet is available. (we just loose syntax highlighting, its not that bad.)
document.addEventListener('DOMContentLoaded', (event) => {
  document.querySelectorAll('pre>code').forEach((block) => {
  	try{
    	hljs.highlightBlock(block);
	}catch(err){ }
  });
});
</script>
</html>
"""

with open(doc_folder + '/index.html','w+') as f:
		f.write(full_doc)
		print("Documentation available at:",doc_folder + '/index.html')