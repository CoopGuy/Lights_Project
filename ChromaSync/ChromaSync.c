typedef struct Light{
	char r;
	char g;
	char b;
}

typedef struct LightString{
	Light* lights;
	uint32_t count;
	uint8_t pin	
} LightString;

*LightString createLightString(uint32_t count, uint8_t pin);
*LightString createLightString(uint32_t count, uint8_t pin, Light defaultVal);

void deleteLightString(*LightString s);

