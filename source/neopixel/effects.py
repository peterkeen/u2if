import random
import colorsys

class BaseEffect:
    def __init__(self, count=1, include_white_channel=False, **kwargs):
        self.count = count
        self.pixels = []
        self.include_white_channel = include_white_channel

        for i in range(count):
            pixel = [0.0, 0.0, 0.0]
            if self.include_white_channel:
                pixel.append(0.0)

            self.pixels.append(pixel)

    def next_frame(self):
        pass

    def update(self, state):
        pass

class StaticEffect(BaseEffect):
    def __init__(self, *args, color=[0.0,0.0,0.0,1.0], **kwargs):
        super().__init__(*args, **kwargs)
        self.color = color
        self.update_pixels()

    def update(self, state):
        self.color[0] = state.red
        self.color[1] = state.green
        self.color[2] = state.blue

        if self.include_white_channel:
            self.color[3] = state.white

        self.update_pixels()

    def update_pixels(self):
        for i in range(self.count):
            self.pixels[i] = self.color

class TwinkleEffect(BaseEffect):
    def __init__(self, *args, frequency=0.05, brighten=0.6, min_threshold=0.2, decay_rate=0.03, **kwargs):
        super().__init__(*args, **kwargs)
        self.frequency = frequency
        self.decay_rate = decay_rate
        self.brighten = brighten
        self.min_threshold = min_threshold

    def next_frame(self):
        for i in range(self.count):
            val = self.pixels[i][0]
            if val < self.min_threshold:
                if random.random() <= self.frequency:
                    val = self.brighten

            val = val - self.decay_rate
            if val < 0.0:
              val = 0.0

            pixel = [val, val, val]
            if self.include_white_channel:
                pixel.append(val)

            self.pixels[i] = pixel

class BlendEffect(BaseEffect):
    # https://github.com/julianschill/klipper-led_effect/blob/master/src/led_effect.py#L324C9-L341C13
    BLENDING_MODES  = {
     'top'       : (lambda t, b: t ),
     'bottom'    : (lambda t, b: b ),
     'add'       : (lambda t, b: t + b ),
     'subtract'  : (lambda t, b: (b - t) * (b - t > 0)),
     'subtract_b': (lambda t, b: (t - b) * (t - b > 0)),
     'difference': (lambda t, b: (t - b) * (t > b) + (b - t) * (t <= b)),
     'average'   : (lambda t, b: 0.5 * (t + b)),
     'multiply'  : (lambda t, b: t * b),
     'divide'    : (lambda t, b: t / b if b > 0 else 0 ),
     'divide_inv': (lambda t, b: b / t if t > 0 else 0 ),
     'screen'    : (lambda t, b: 1.0 - (1.0-t)*(1.0-b) ),
     'lighten'   : (lambda t, b: t * (t > b) +  b * (t <= b)),
     'darken'    : (lambda t, b: t * (t < b) +  b * (t >= b)),
     'overlay'   : (lambda t, b: \
                         2.0 * t * b if t > 0.5 else \
                         1.0 - (2.0 * (1.0-t) * (1.0-b)))
    }

    def __init__(self, top, bottom, mode='top', **kwargs):
        self.top = top
        self.bottom = bottom
        self.mode = mode
        super().__init__(top.count, **kwargs)

    def update(self, state):
        self.top.update(state)
        self.bottom.update(state)

    def next_frame(self):
        self.top.next_frame()
        self.bottom.next_frame()

        blend = self.BLENDING_MODES[self.mode]
        for i in range(self.count):
            result_pixel = []
            top_pixel = self.top.pixels[i]
            bottom_pixel = self.bottom.pixels[i]

            for p in range(len(top_pixel)):
                result_pixel.append(blend(top_pixel[p], bottom_pixel[p]))

            self.pixels[i] = result_pixel


class RainbowEffect(BaseEffect):
    def __init__(self, *args, brightness=0.05, **kwargs):
        super().__init__(*args, **kwargs)

        for i in range(num_pixels):
            rgb = colorsys.hsv_to_rgb(i/(num_pixels + 0.0), 1.0, brightness)
            self.pixels[i] = [rgb[0], rgb[1], rgb[2]]
            if self.include_white_channel:
                self.pixels[i][3] = 0.0

    def next_frame(self):
        for i in range(len(self.pixels)):
            p = self.pixels[i]
            hsv = list(colorsys.rgb_to_hsv(*p))
            hsv[0] += 0.005
            rgb = colorsys.hsv_to_rgb(*hsv)
            self.pixels[i] = list(rgb)
            if self.include_white_channel:
                self.pixels[i][3] = 0.0
