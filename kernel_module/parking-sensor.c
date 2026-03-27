#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>

#define DRIVER_NAME "parking_sensor"
#define TRIG_GPIO1 536  // BCM GPIO24 (Pin 18) for sensor 1
#define ECHO_GPIO1 535  // BCM GPIO23 (Pin 16) for sensor 1
#define TRIG_GPIO2 537  // BCM GPIO25 (Pin 22) for sensor 2
#define ECHO_GPIO2 538  // BCM GPIO8 (Pin 24) for sensor 2
#define LED_GPIO1 539   // BCM GPIO17 (Pin 4) for LED 1 (Vacant)
#define LED_GPIO2 540   // BCM GPIO27 (Pin 12) for LED 2 (Occupied)

static dev_t parking_sensor_dev;
static struct cdev parking_sensor_cdev;
static struct kobject *parking_sensor_kobj;
static DEFINE_MUTEX(parking_sensor_mutex);
static ktime_t rising1, falling1, rising2, falling2;
static bool slot1_occupied = false, slot2_occupied = false;

static struct gpio_desc *trig_desc1, *echo_desc1, *trig_desc2, *echo_desc2, *led_desc1, *led_desc2;

static ssize_t parking_sensor_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t parking_sensor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobj_attribute parking_sensor_attr = __ATTR(status, 0644, parking_sensor_show, parking_sensor_store);

static irqreturn_t echo_irq_handler1(int irq, void *dev_id)
{
    if (gpiod_get_value(echo_desc1)) {
        rising1 = ktime_get();
    } else {
        falling1 = ktime_get();
        slot1_occupied = (ktime_to_ns(ktime_sub(falling1, rising1)) / 58) < 100;  // Example threshold
    }
    return IRQ_HANDLED;
}

static irqreturn_t echo_irq_handler2(int irq, void *dev_id)
{
    if (gpiod_get_value(echo_desc2)) {
        rising2 = ktime_get();
    } else {
        falling2 = ktime_get();
        slot2_occupied = (ktime_to_ns(ktime_sub(falling2, rising2)) / 58) < 100;  // Example threshold
    }
    return IRQ_HANDLED;
}

static int parking_sensor_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&parking_sensor_mutex))
        return -EBUSY;
    return 0;
}

static int parking_sensor_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&parking_sensor_mutex);
    return 0;
}

static ssize_t parking_sensor_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    char status[256];
    snprintf(status, sizeof(status), "Slot 1: %s\nSlot 2: %s\n", 
             slot1_occupied ? "Occupied" : "Vacant", 
             slot2_occupied ? "Occupied" : "Vacant");
    
    if (copy_to_user(buf, status, strlen(status)))
        return -EFAULT;
    return strlen(status);
}

static ssize_t parking_sensor_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    gpiod_set_value(trig_desc1, 1);
    gpiod_set_value(trig_desc2, 1);
    udelay(10);
    gpiod_set_value(trig_desc1, 0);
    gpiod_set_value(trig_desc2, 0);
    return count;
}

static ssize_t parking_sensor_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return parking_sensor_read(NULL, buf, 0, NULL);
}

static ssize_t parking_sensor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return parking_sensor_write(NULL, buf, count, NULL);
}

static struct file_operations parking_sensor_fops = {
    .owner = THIS_MODULE,
    .open = parking_sensor_open,
    .release = parking_sensor_release,
    .read = parking_sensor_read,
    .write = parking_sensor_write,
};

static int __init parking_sensor_init(void)
{
    int ret;
    int irq_number1, irq_number2;

    // GPIO setup using descriptors
    trig_desc1 = gpio_to_desc(TRIG_GPIO1);
    echo_desc1 = gpio_to_desc(ECHO_GPIO1);
    trig_desc2 = gpio_to_desc(TRIG_GPIO2);
    echo_desc2 = gpio_to_desc(ECHO_GPIO2);
    led_desc1 = gpio_to_desc(LED_GPIO1);
    led_desc2 = gpio_to_desc(LED_GPIO2);

    if (!trig_desc1 || !echo_desc1 || !trig_desc2 || !echo_desc2 || !led_desc1 || !led_desc2) {
        pr_err("Failed to get GPIO descriptors\n");
        return -ENODEV;
    }

    // Set GPIO directions
    ret = gpiod_direction_output(trig_desc1, 0);
    if (ret) return ret;
    ret = gpiod_direction_input(echo_desc1);
    if (ret) return ret;

    ret = gpiod_direction_output(trig_desc2, 0);
    if (ret) return ret;
    ret = gpiod_direction_input(echo_desc2);
    if (ret) return ret;

    ret = gpiod_direction_output(led_desc1, 0);  // Initially turned off
    if (ret) return ret;
    ret = gpiod_direction_output(led_desc2, 0);  // Initially turned off
    if (ret) return ret;

    // IRQ setup
    irq_number1 = gpio_to_irq(ECHO_GPIO1);
    irq_number2 = gpio_to_irq(ECHO_GPIO2);

    ret = request_irq(irq_number1, echo_irq_handler1, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "parking_sensor_echo1", NULL);
    if (ret) return ret;

    ret = request_irq(irq_number2, echo_irq_handler2, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "parking_sensor_echo2", NULL);
    if (ret) return ret;

    // Character device setup
    if ((ret = alloc_chrdev_region(&parking_sensor_dev, 0, 1, DRIVER_NAME)))
        return ret;

    cdev_init(&parking_sensor_cdev, &parking_sensor_fops);
    parking_sensor_cdev.owner = THIS_MODULE;
    ret = cdev_add(&parking_sensor_cdev, parking_sensor_dev, 1);
    if (ret) return ret;

    // Sysfs setup
    parking_sensor_kobj = kobject_create_and_add("parking_sensor", kernel_kobj);
    if (!parking_sensor_kobj) {
        pr_err("Failed to create kobject\n");
        return -ENOMEM;
    }

    ret = sysfs_create_file(parking_sensor_kobj, &parking_sensor_attr.attr);
    if (ret) return ret;

    pr_info("Parking Sensor module loaded\n");
    return 0;
}

static void __exit parking_sensor_exit(void)
{
    free_irq(gpio_to_irq(ECHO_GPIO1), NULL);
    free_irq(gpio_to_irq(ECHO_GPIO2), NULL);

    gpiod_put(echo_desc1);
    gpiod_put(trig_desc1);
    gpiod_put(echo_desc2);
    gpiod_put(trig_desc2);
    gpiod_put(led_desc1);
    gpiod_put(led_desc2);

    sysfs_remove_file(parking_sensor_kobj, &parking_sensor_attr.attr);
    kobject_put(parking_sensor_kobj);

    cdev_del(&parking_sensor_cdev);
    unregister_chrdev_region(parking_sensor_dev, 1);

    pr_info("Parking Sensor module unloaded\n");
}

module_init(parking_sensor_init);
module_exit(parking_sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gouri");
MODULE_DESCRIPTION("Parking Slot Monitoring with HC-SR04 Ultrasonic Sensors and LEDs");
