from PIL import Image, ImageDraw, ImageFont

size = 200, 200
position = (0,0)
test_text = 'hello world'

img = Image.new("RGB", size, "white")
draw = ImageDraw.Draw(img)
font = ImageFont.load_default()

bbox = draw.textbbox(position, test_text, font, )
draw.text(position, test_text, fill = 'black', font= font)
draw.rectangle(bbox, outline='red')

img.show()