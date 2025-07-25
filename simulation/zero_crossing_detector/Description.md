# مدار تشخیص عبور از صفر (AC Zero-Crossing Detector)

## مقدمه: چرا به تشخیص عبور از صفر نیاز داریم؟

در سیستم‌های قدرت AC، ولتاژ به صورت یک موج سینوسی با فرکانس مشخص (مثلاً 50 یا 60 هرتز) نوسان می‌کند. **"عبور از صفر"** لحظه‌ای است که دامنه این ولتاژ از مقدار صفر عبور می‌کند. با دانستن این لحظه می‌توانیم در **کنترل زاویه فاز (Phase Angle Control)**، نقطه‌ی‌ شروع هر نیم‌سیکل را تشخیص‌دهیم و با یک تأخیر معین، یک عنصر سوئیچینگ مانند **ترایاک (TRIAC)** را فعال کنیم. با کنترل این تأخیر، می‌توانیم توان متوسط تحویلی به بار را کنترل کنیم. این اساس کار دیمرهای نوری است.

---

## تحلیل گام به گام مدار

این مدار به صورت منطقی به چهار بخش اصلی تقسیم می‌شود که هر کدام وظیفه‌ی مشخصی دارند.

### چرا این قطعات استفاده شده‌اند؟

* **مقاومت‌های سری (R2, R3, R5, R4):** ولتاژ برای اتصال مستقیم به قطعاتی مانند LED اپتوکوپلر بسیار بالاست. این شبکه مقاومتی (با مقاومت کلی بالا) دو کار انجام می‌دهد:
    1.  **محدود کردن جریان:** طبق **قانون اهم (V=IR)**، مقاومت بالا جریان را به شدت کاهش می‌دهد (در حد چند میلی‌آمپر) تا به دیود پل و اپتوکوپلر آسیب نرسد.
    2.  **تقسیم توان:** به جای استفاده از یک مقاومت با توان بالا، از چهار مقاومت استفاده شده تا توان تلفاتی **(P=I²R)** بین آن‌ها تقسیم شود و از داغ شدن بیش از حد یک قطعه جلوگیری شود.

* **فیلتر RC (R1, C1):** این ترکیب یک **فیلتر پایین‌گذر (Low-Pass Filter)** ساده است. خطوط برق شهری معمولاً دارای نویز و اسپایک‌های فرکانس بالا هستند. این فیلتر کمک می‌کند تا این نویزها قبل از رسیدن به بخش تشخیص، تضعیف شوند و عملکرد مدار پایدارتر باشد.

---

## بخش یک‌سوسازی: آماده‌سازی سیگنال برای اپتوکوپلر

* **قطعه:** BR1 دیود پل یا Bridge Rectifier
* **هدف:** تبدیل موج متناوب AC به یک موج DC پالسی (یک‌سوشده تمام-موج).

#### چرا به یک‌سوسازی نیاز است؟
LED داخل اپتوکوپلر یک دیود است و فقط در یک جهت جریان را عبور می‌دهد. اگر موج AC را مستقیماً به آن وصل کنیم، LED فقط در نیم‌سیکل‌های مثبت روشن می‌شود و ما تشخیص عبور از صفر را در نیم‌سیکل‌های منفی از دست می‌دهیم. دیود پل با "برگرداندن" نیم‌سیکل منفی، باعث می‌شود که در هر دو نیم‌سیکل، جریانی با پلاریته‌ی صحیح به اپتوکوپلر برسد. در نتیجه، ما در هر سیکل AC، دو بار لحظه‌ی عبور از صفر را تشخیص می‌دهیم (فرکانس تشخیص 100 هرتز برای برق 50 هرتز).

#### خروجی پل چیست ؟
خروجی دیود پل دیگر یک موج AC سینوسی نیست. در واقع، خروجی این بخش، **مقدار مطلق (Absolute Value)** موج ورودی است. تصور کنید نمودار ولتاژ AC را روی کاغذ دارید. خروجی دیود پل معادل این است که شما تمام قسمت‌های نمودار را که زیر محور صفر هستند (بخش‌های منفی) ببُرید و به صورت قرینه به بالای محور صفر منتقل کنید.

به این سیگنال خروجی به صورت تخصصی **"DC پالسی" (Pulsating DC)** یا **"سیگنال یک‌سوشده‌ی تمام-موج" (Full-Wave Rectified Signal)** می‌گویند. ویژگی‌های کلیدی آن این‌هاست:

1.  **پلاریته ثابت:** ولتاژ آن همیشه مثبت است و هرگز به زیر صفر نمی‌رود. به همین دلیل به آن DC می‌گوییم.
2.  **مقدار متغیر:** دامنه ولتاژ آن ثابت نیست، بلکه مانند تپه‌هایی پشت سر هم، به طور مداوم از صفر به یک مقدار حداکثر (پیک) و دوباره به صفر کاهش می‌یابد. به همین دلیل به آن "پالسی" می‌گوییم.
3.  **فرکانس دو برابر:** تعداد این "تپه‌ها" یا پالس‌ها در هر ثانیه، دو برابر فرکانس برق شهر است. اگر برق ورودی 50 هرتز باشد، سیگنال خروجی 100 پالس در ثانیه خواهد داشت.

---

## بخش ایزولاسیون: قلب ایمنی مدار

* **قطعات:** U1 (PC817A), R6
* **هدف:** **جداسازی الکتریکی (گالوانیک)** کامل بین بخش ولتاژ بالا (AC) و بخش ولتاژ پایین (مدار کنترلی).

مفهوم **ایزولاسیون گالوانیک**: اپتوکوپلر از یک LED و یک فتوترانزیستور تشکیل شده که در یک پکیج قرار دارند اما هیچ اتصال الکتریکی مستقیمی بین آن‌ها وجود ندارد. ارتباط فقط از طریق نور برقرار می‌شود. این ویژگی یک سد فیزیکی بین ولتاژ خطرناک AC و میکروکنترلر حساس شما ایجاد می‌کند و مانع از آسیب دیدن آن در صورت بروز خطا در بخش AC می‌شود.

#### نحوه عملکرد:
1.  جریان یک‌سوشده از دیود پل، از طریق مقاومت R6 به LED اپتوکوپلر می‌رسد. R6 جریان را در حدی نگه می‌دارد که LED به طور مطمئن روشن شود اما نسوزد.
2.  تا زمانی که ولتاژ لحظه‌ای AC از صفر فاصله دارد، جریان کافی برای روشن نگه داشتن LED وجود دارد. نور LED، فتوترانزیستور را روشن (اشباع) می‌کند.
3.  هنگامی که ولتاژ AC به نزدیکی صفر می‌رسد، جریان ورودی برای روشن کردن LED کافی نیست. LED خاموش شده و در نتیجه فتوترانزیستور نیز خاموش می‌شود.

---

## بخش خروجی: تولید سیگنال دیجیتال استاندارد

* **قطعات:** فتوترانزیستورِ U1, R7
* **هدف:** تبدیل حالت روشن/خاموش فتوترانزیستور به یک سیگنال ولتاژ تمیز (0 ولت یا 5 ولت).

#### عملکرد:
* **وقتی فتوترانزیستور خاموش است (لحظه عبور از صفر):** مسیر به زمین قطع است. مقاومت R7 پین خروجی را به +5V "می‌کشد" و سطح ولتاژ خروجی **HIGH** می‌شود.
* **وقتی فتوترانزیستور روشن است:** مانند یک کلید بسته عمل کرده و پین خروجی را مستقیماً به زمین متصل می‌کند. در نتیجه، ولتاژ خروجی **LOW** (نزدیک به صفر) می‌شود.

**خلاصه عملکرد کلی:** خروجی مدار در حالت عادی **LOW** است و فقط در لحظات بسیار کوتاه عبور از صفر، یک پالس **HIGH** تولید می‌کند.

---

## لیست قطعات (Bill of Materials - BOM)

| نام قطعه      | مقدار      | توضیحات                                       |
| :------------ | :--------- | :--------------------------------------------- |
| **R6** | 1kΩ        | مقاومت محدودکننده جریان LED اپتوکوپلر          |
| **R2,R3,R4,R5**| 10kΩ       | شبکه تقسیم ولتاژ و محدودکننده جریان ورودی     |
| **R7** | 4.7kΩ      | مقاومت Pull-up برای خروجی                     |
| **R1** | 100Ω       | مقاومت فیلتر ورودی                            |
| **C1** | 100nF      | خازن فیلتر ورودی                              |
| **BR1** | -          | دیود پل                                        |
| **U1** | PC817A     | اپتوکوپلر                                      |
| **V1** | 230\*1.41V | منبع ولتاژ متناوب                             |