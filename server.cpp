#include "server.h"

const int MAX_COM_ID = 20;

Server::Server(QObject *parent) : QObject(parent)
{
    sendTimer = new QTimer(this);
    connect(sendTimer, SIGNAL(timeout()), this, SLOT( sendMessage() ));
    newPort = 0;
    nextMessageType = 0xA5;
    currentMessageType = 0xA5;
    msg_lost_counter = 0;
    msg_received_counter = 0;
}

bool Server::COMconnect(int com_num)
{
    int isOpened = false;

    QString str = "COM";
    str.append(QString::number(com_num));

    std::cout << "Trying to open port |" << str.toStdString() << "|...";

    newPort = new QSerialPort(str);
    newPort->setBaudRate(settings->connection->baudRate, QSerialPort::AllDirections);
    newPort->setDataBits(settings->connection->dataBits);
    newPort->setParity(settings->connection->parity);
    newPort->setStopBits(settings->connection->stopBits);
    newPort->setFlowControl(settings->connection->flowControl);

    try {
        isOpened = newPort->open(QIODevice::ReadWrite);
    } catch(...) {
        std::cout << " serial port openning error" << std::endl;
        emit info("Serial port openning error");
        return false;
    }

    if (isOpened) {
        std::cout << " successfully opened!" << std::endl;
        sendTimer->start(REQUEST_DELAY);
    } else {
        std::cout << ". Unable to open serial port" << std::endl;
        emit info("Unable to open serial port");
        return false;
    }
    return true;
}


void Server::sendMessage() {
    if (currentMessageType != nextMessageType)
        currentMessageType = nextMessageType;
    switch (currentMessageType) {
    case REQUEST_NORMAL_CODE:
        sendMessageNormal();
        break;
    case REQUEST_CONFIG_CODE:
        sendMessageConfig();
        break;
    case REQUEST_DIRECT_CODE:
        sendMessageDirect();
        break;
    default:
        std::cout << "ERROR: Unknown current message code!" << std::endl;
    }
}

void Server::sendMessageNormal()
{
    for (int i = 0; i < REQUEST_NORMAL_LENGTH; ++i) {
        msg_to_send[i] = 0x00;
    }
    //msg_to_send[0] = 0xFF;
    j->update();

    msg_to_send[REQUEST_NORMAL_TYPE] = REQUEST_NORMAL_CODE;

    msg_to_send[REQUEST_NORMAL_MARCH+1] = j->march;
    msg_to_send[REQUEST_NORMAL_MARCH] = j->march >> 8;

    msg_to_send[REQUEST_NORMAL_LAG+1] = j->lag;
    msg_to_send[REQUEST_NORMAL_LAG] = j->lag >> 8;

    msg_to_send[REQUEST_NORMAL_DEPTH+1] = j->depth;
    msg_to_send[REQUEST_NORMAL_DEPTH] = j->depth >> 8;

    msg_to_send[REQUEST_NORMAL_ROLL+1] = j->roll;
    msg_to_send[REQUEST_NORMAL_ROLL] = j->roll >> 8;

    msg_to_send[REQUEST_NORMAL_PITCH+1] = j->pitch;
    msg_to_send[REQUEST_NORMAL_PITCH] = j->pitch >> 8;

    msg_to_send[REQUEST_NORMAL_YAW+1] = j->yaw;
    msg_to_send[REQUEST_NORMAL_YAW] = j->yaw >> 8;


    msg_to_send[REQUEST_NORMAL_LIGHT]      = j->light;
    msg_to_send[REQUEST_NORMAL_GRAB]       = j->grab;
    msg_to_send[REQUEST_NORMAL_BT]       = j->bt;
    msg_to_send[REQUEST_NORMAL_BOTTOM_LIGHT] = j->bottom_light;
    msg_to_send[REQUEST_NORMAL_BT] = j->agar;
    msg_to_send[REQUEST_NORMAL_TILT] = j->tilt;
    msg_to_send[REQUEST_NORMAL_ROTATE] = 1.27*(float)j->grab_rotate;

    msg_to_send[REQUEST_NORMAL_STABILIZE_DEPTH] = j->stabilize_depth;
    msg_to_send[REQUEST_NORMAL_STABILIZE_ROLL] = j->stabilize_roll;
    msg_to_send[REQUEST_NORMAL_STABILIZE_PITCH] = j->stabilize_pitch;
    msg_to_send[REQUEST_NORMAL_STABILIZE_YAW] = false;
    msg_to_send[REQUEST_NORMAL_RESET_IMU] = false;

    addCheckSumm16b(msg_to_send, REQUEST_NORMAL_LENGTH);

    //std::cout << "Sending NORMAL message:" << std::endl;
    /*for (int i = 0; i < REQUEST_NORMAL_LENGTH; ++i) {
        std::cout << "|N" << i << "=" << unsigned(msg_to_send[i]) << std::endl;
    }*/

    newPort->write((char*)msg_to_send, REQUEST_NORMAL_LENGTH);

    emit imSleeping();

    QTest::qSleep (settings->connection->pause_after_sent);
    receiveMessage();
}

void Server::sendMessageDirect() {
    for (int i = 0; i < REQUEST_DIRECT_LENGTH; ++i) {
        msg_to_send[i] = 0x00;
    }
    msg_to_send[0] = 0xFF;

    msg_to_send[REQUEST_DIRECT_TYPE] = REQUEST_DIRECT_CODE;

    msg_to_send[REQUEST_DIRECT_1] = settings->motors[0].speed;
    msg_to_send[REQUEST_DIRECT_2] = settings->motors[1].speed;
    msg_to_send[REQUEST_DIRECT_3] = settings->motors[2].speed;
    msg_to_send[REQUEST_DIRECT_4] = settings->motors[3].speed;
    msg_to_send[REQUEST_DIRECT_5] = settings->motors[4].speed;
    msg_to_send[REQUEST_DIRECT_6] = settings->motors[5].speed;
    msg_to_send[REQUEST_DIRECT_7] = settings->motors[6].speed;
    msg_to_send[REQUEST_DIRECT_8] = settings->motors[7].speed;

    addCheckSumm16b(msg_to_send, REQUEST_DIRECT_LENGTH);

    //std::cout << "Sending DIRECT message:" << std::endl;
    for (int i = 0; i < REQUEST_DIRECT_LENGTH; ++i) {
        std::cout << "|N" << i << "=" << unsigned(msg_to_send[i]) << std::endl;
    }

    newPort->write((char*)msg_to_send, REQUEST_DIRECT_LENGTH);

    emit imSleeping();
    QTest::qSleep (settings->connection->pause_after_sent);
    receiveMessage();
}

void Server::sendMessageConfig() {
    for (int i = 0; i < REQUEST_CONFIG_LENGTH; ++i) {
        msg_to_send[i] = 0x00;
    }
    msg_to_send[0] = 0xFF;
    msg_to_send[REQUEST_CONFIG_TYPE] = REQUEST_CONFIG_CODE;

    msg_to_send[REQUEST_CONFIG_CONST_TIME_DEPTH]    = settings->depth.const_time;
    msg_to_send[REQUEST_CONFIG_K1_DEPTH]            = settings->depth.k1;
    msg_to_send[REQUEST_CONFIG_K2_DEPTH]            = settings->depth.k2;
    msg_to_send[REQUEST_CONFIG_START_DEPTH]         = settings->depth.start;
    msg_to_send[REQUEST_CONFIG_GAIN_DEPTH]          = settings->depth.gain;

    msg_to_send[REQUEST_CONFIG_CONST_TIME_ROLL]     = settings->roll.const_time;
    msg_to_send[REQUEST_CONFIG_K1_ROLL]             = settings->roll.k1;
    msg_to_send[REQUEST_CONFIG_K2_ROLL]             = settings->roll.k2;
    msg_to_send[REQUEST_CONFIG_START_ROLL]          = settings->roll.start;
    msg_to_send[REQUEST_CONFIG_GAIN_ROLL]           = settings->roll.gain;

    msg_to_send[REQUEST_CONFIG_CONST_TIME_PITCH]    = settings->pitch.const_time;
    msg_to_send[REQUEST_CONFIG_K1_PITCH]            = settings->pitch.k1;
    msg_to_send[REQUEST_CONFIG_K2_PITCH]            = settings->pitch.k2;
    msg_to_send[REQUEST_CONFIG_START_PITCH]         = settings->pitch.start;
    msg_to_send[REQUEST_CONFIG_GAIN_PITCH]          = settings->pitch.gain;

    msg_to_send[REQUEST_CONFIG_CONST_TIME_YAW]      = settings->yaw.const_time;
    msg_to_send[REQUEST_CONFIG_K1_YAW]              = settings->yaw.k1;
    msg_to_send[REQUEST_CONFIG_K2_YAW]              = settings->yaw.k2;
    msg_to_send[REQUEST_CONFIG_START_YAW]           = settings->yaw.start;
    msg_to_send[REQUEST_CONFIG_GAIN_YAW]            = settings->yaw.gain;

    settings->motors;

    for (int i = 0; i < 8; ++i) {
        switch (settings->motors[i].code) {
        case settings->motors[i].HLB:
            msg_to_send[REQUEST_CONFIG_POSITION_HLB] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_HLB]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HLB, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HLB,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HLB, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HLB,  0.0);
            }
            break;
        case settings->motors[i].HLF:
            msg_to_send[REQUEST_CONFIG_POSITION_HLF] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_HLF]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HLF, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HLF,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HLF, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HLF,  0.0);
            }
            break;
        case settings->motors[i].HRB:
            msg_to_send[REQUEST_CONFIG_POSITION_HRB] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_HRB]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HRB, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HRB,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HRB, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HRB,  0.0);
            }
            break;
        case settings->motors[i].HRF:
            msg_to_send[REQUEST_CONFIG_POSITION_HRF] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_HRF]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HRF, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HRF,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_HRF, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_HRF,  0.0);
            }
            break;
        case settings->motors[i].VB:
            msg_to_send[REQUEST_CONFIG_POSITION_VB] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_VB]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VB, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VB,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VB, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VB,  0.0);
            }
            break;
        case settings->motors[i].VF:
            msg_to_send[REQUEST_CONFIG_POSITION_VF] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_VF]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VF, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VF,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VF, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VF,  0.0);
            }
            break;
        case settings->motors[i].VL:
            msg_to_send[REQUEST_CONFIG_POSITION_VL] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_VL]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VL, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VL,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VL, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VL,  0.0);
            }
            break;
        case settings->motors[i].VR:
            msg_to_send[REQUEST_CONFIG_POSITION_VR] = i;
            msg_to_send[REQUEST_CONFIG_INVERSE_VR]  = settings->motors[i].inverse;
            if (settings->motors[i].enabled) {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VR, settings->motors[i].k_backward);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VR,  settings->motors[i].k_forward);
            } else {
                addFloat(msg_to_send, REQUEST_CONFIG_K_BACKWARD_VR, 0.0);
                addFloat(msg_to_send, REQUEST_CONFIG_K_FORWARD_VR,  0.0);
            }
            break;
        default:
            std::cout << "ERROR: unknown motor code!" << std::endl;
        }

    }

    addCheckSumm16b(msg_to_send, REQUEST_CONFIG_LENGTH);

    //std::cout << "Sending CONFIG message:" << std::endl;
    //for (int i = 0; i < REQUEST_CONFIG_LENGTH; ++i) {
    //    std::cout << "|N" << i << "=" << unsigned(msg_to_send[i]) << std::endl;
    //}

    newPort->write((char*)msg_to_send, REQUEST_CONFIG_LENGTH);

    emit imSleeping();
    QTest::qSleep (settings->connection->pause_after_sent);
    receiveMessage();
}



void Server::receiveMessage() {
    newPort->waitForReadyRead(25);
    int buffer_size = newPort->bytesAvailable();

    std::cout << "In input buffer there are " << buffer_size << " bytes availible" << std::endl;


    if (emulation_mode){
        std::cout << "WARNING: Emulation mode" << std::endl;

        imu_roll = imu_roll + j->roll/10000;
        imu_pitch = imu_pitch + j->pitch/10000;
        imu_yaw = imu_yaw + j->yaw/10000;

        imu_roll_speed = j->roll/10000;
        imu_pitch_speed = j->pitch/10000;
        imu_yaw_speed = j->yaw/10000;

        imu_depth = imu_depth + j->depth/10000;

        current_HLB = 1;
        current_HLF = 11;
        current_HRB = 111;
        current_HRF = 1111;
        current_VB = 11111;
        current_VF = 234;
        current_VL = 4567;
        current_VR = 7808;

        velocity_HLB = 87;
        velocity_HLF = 89;
        velocity_HRB = 90;
        velocity_HRF = 35;
        velocity_VB = 76;
        velocity_VF = 88;
        velocity_VL = 99;
        velocity_VR = 999;

        current_light = 5;
        current_bottom_light = 10;
        current_agar = 15;
        current_grab = 20;
        current_grab_rotate = 25;
        current_tilt = 30;

        bt_data = "testMSG";


    } else if (buffer_size == 0) {
        std::cout << "No message to read. Buffer is empty" << std::endl;
        msg_lost_counter++;
    } else {
        int counter = 0;
        while (true) {
            int bytesAvailible = newPort->bytesAvailable();
            if (bytesAvailible % RESPONSE_LENGTH == 0) {
                break;
            } else if (counter > 250000) {
                newPort->clear();
            } else {
                counter++;
            }
        }
        msg_in = newPort->readAll();

        std::cout << "Got response. First symbol: " << msg_in[0] << std::endl;
        std::cout << "Checksum...";

        if (isCheckSumm16bCorrect((uint8_t*) msg_in.data(), RESPONSE_LENGTH)) {
            std::cout << "OK" << std::endl;
            msg_received_counter++;
        } else {
            std::cout << "INCORRECT!" << std::endl;
            msg_lost_counter++;
            return;
        }

        imu_roll = (int16_t) (msg_in[RESPONSE_ROLL]) << 8 | msg_in[RESPONSE_ROLL+1];
        imu_pitch = (int16_t) (msg_in[RESPONSE_PITCH]) << 8 | msg_in[RESPONSE_PITCH+1];
        imu_yaw = (int16_t) (msg_in[RESPONSE_YAW]) << 8 | msg_in[RESPONSE_YAW+1];

        imu_roll_speed = (int16_t) (msg_in[RESPONSE_ROLL_SPEED]) << 8 | msg_in[RESPONSE_ROLL_SPEED+1];
        imu_pitch_speed = (int16_t) (msg_in[RESPONSE_PITCH_SPEED]) << 8 | msg_in[RESPONSE_PITCH_SPEED+1];
        imu_yaw_speed = (int16_t) (msg_in[RESPONSE_YAW_SPEED]) << 8 | msg_in[RESPONSE_YAW_SPEED+1];

        imu_depth = msg_in[RESPONSE_PRESSURE]; // Pressure вместо depth

        current_HLB = msg_in[RESPONSE_VMA_CURRENT_HLB];
        current_HLF = msg_in[RESPONSE_VMA_CURRENT_HLF];
        current_HRB = msg_in[RESPONSE_VMA_CURRENT_HRB];
        current_HRF = msg_in[RESPONSE_VMA_CURRENT_HRF];
        current_VB = msg_in[RESPONSE_VMA_CURRENT_VB];
        current_VF = msg_in[RESPONSE_VMA_CURRENT_VF];
        current_VL = msg_in[RESPONSE_VMA_CURRENT_VL];
        current_VR = msg_in[RESPONSE_VMA_CURRENT_VR];

        velocity_HLB = msg_in[RESPONSE_VMA_VELOCITY_HLB];
        velocity_HLF = msg_in[RESPONSE_VMA_VELOCITY_HLF];
        velocity_HRB = msg_in[RESPONSE_VMA_VELOCITY_HRB];
        velocity_HRF = msg_in[RESPONSE_VMA_VELOCITY_HRF];
        velocity_VB = msg_in[RESPONSE_VMA_VELOCITY_VB];
        velocity_VF = msg_in[RESPONSE_VMA_VELOCITY_VF];
        velocity_VL = msg_in[RESPONSE_VMA_VELOCITY_VL];
        velocity_VR = msg_in[RESPONSE_VMA_VELOCITY_VR];

        current_light = msg_in[RESPONSE_DEV_CURRENT_1];
        current_bottom_light = msg_in[RESPONSE_DEV_CURRENT_2];
        current_agar = msg_in[RESPONSE_DEV_CURRENT_3];
        current_grab = msg_in[RESPONSE_DEV_CURRENT_4];
        current_grab_rotate = msg_in[RESPONSE_DEV_CURRENT_5];
        current_tilt = msg_in[RESPONSE_DEV_CURRENT_6];


        err_vma = msg_in[RESPONSE_VMA_ERRORS];
        err_dev = msg_in[RESPONSE_DEV_ERRORS];


        // !!!!! СДВИГИ, ЧТОБЫ ОБРАЗОВАТЬ ПРАВИЛЬНОЕ ЧИСЛО

        char bt[8];  //Важно, чтобы история не затерлась! -> логи, выгружаемые на другое устройство
        for (int i = 0; i < 8; ++i) {
            bt[i] = msg_in[RESPONSE_AGAR + i];
        }

        QString s(bt);
        bt_data = s;

        //bt_data = QString::fromAscii_helper(bt, 7);

        //temperature = encodeTemperature(temperature_MS, temperature_LS);

        int16_t pressure = msg_in[RESPONSE_PRESSURE];

        uint16_t motor_errors = msg_in[RESPONSE_VMA_ERRORS];



        /*
        std::cout << "Received  data:" << std::endl;
        std::cout << "roll " << imu_roll << "pitch " << imu_pitch << "yaw " << imu_yaw << std::endl;
        std::cout << "roll_speed" << imu_roll_speed << "pitch_speed" << imu_pitch_speed << "yaw_speed" << imu_yaw_speed << std::endl;
        std::cout << "temperature=" << temperature << " pressure=" << pressure << std::endl;
        std::cout << "bluetooth: " << bt << std::endl;
        std::cout << "motor_errors" << motor_errors << std::endl;
        */
    }
    //QTest::qSleep (settings->connection->pause_after_received);
    msg_lost_percent = (float) msg_lost_counter / ((float)msg_received_counter+ (float)msg_lost_counter);
    msg_lost_percent *= 100;

    std::cout << "RECEIVED = " << msg_received_counter << std::endl;
    std::cout << "LOST = " << msg_lost_counter << std::endl;
    std::cout << "LOST PERCENT = " << msg_lost_percent << std::endl;
}


/* CRC16-CCITT algorithm */
uint8_t Server::isCheckSumm16bCorrect(uint8_t * msg, uint16_t length)
{
    uint16_t crcGot, crc = 0;
    int i;

    crcGot = (uint16_t)( msg[length-1] + (msg[length-2] << 8) );

    for(i=0; i < length - 2; i++){ //i теперь не с 0, а с 1
        crc = (uint8_t)(crc >> 8) | (crc << 8);
        crc ^= msg[i];
        crc ^= (uint8_t)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }

    if(crc == crcGot )
        return 1;
    else return 0;
}

void Server::addCheckSumm16b(uint8_t * msg, uint16_t length)//i теперь не с 0, а с 1
{
    uint16_t crc = 0;
    int i = 0;

    for(i=0; i < length - 2; i++){
        crc = (uint8_t)(crc >> 8) | (crc << 8);
        crc ^= msg[i];
        crc ^= (uint8_t)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }

    msg[length-2] = (uint8_t) (crc >> 8);
    msg[length-1] = (uint8_t) crc;
}

void Server::addFloat(uint8_t * msg, int position, float value) {
    memcpy(msg + position, (unsigned char*) (&value), 4);
}


void Server::connect_fake() {
    std::cout << "Warning! Overriding COM port!" << std::endl;
    newPort = new QSerialPort();
    sendTimer->start(300);
}

void Server::connect_com()
{
    for (int i = 0; i < MAX_COM_ID; ++i) {
        if (COMconnect(i)){
            break;
        }
    }
}

void Server::disconnect_com() {
    std::cout << "Disconnecting" << std::endl;
    sendTimer->stop();
    newPort->disconnect();
    newPort->close();
    newPort->deleteLater();
}

void Server::addSNP(uint8_t * msg) {
    msg[0] = 'S';
    msg[1] = 'N';
    msg[2] = 'P';
}

/*float Server::encodeTemperature(uint8_t MS, uint8_t LS) {
    uint8_t sign_bytes = MS >> 3;
    int sign = 0;
    if (sign_bytes == 0) {
        sign = -1;
    } else {
        sign = 1;
    }
    int integer_part = ((MS << 5) >> 1) + (LS >> 4);
    float temperature = 0;
    uint8_t bit_power_minus_1 = LS & 0b00001000;
    uint8_t bit_power_minus_2 = LS & 0b00000100;
    uint8_t bit_power_minus_3 = LS & 0b00000010;
    uint8_t bit_power_minus_4 = LS & 0b00000001;
    temperature += integer_part;
    if (bit_power_minus_1) {
        temperature += 0.5;
        std::cout << "1/2 DETECTED!!!11" << std::endl;
    }
    if (bit_power_minus_2) temperature += 0.25;
    if (bit_power_minus_3) temperature += 0.125;
    if (bit_power_minus_4) temperature += 0.0625;
    return temperature;
}*/

Server::~Server() {
    std::cout << "Server shutting down..." << std::endl;
    if (newPort && newPort->isOpen()) {
        std::cout << "Stopping motors.." << std::endl;
        uint8_t msg_to_send[REQUEST_NORMAL_LENGTH];
        for (int i = 0; i < REQUEST_NORMAL_LENGTH; ++i) {
            msg_to_send[i] = 0x00;
        }
        addCheckSumm16b(msg_to_send, REQUEST_NORMAL_LENGTH);
        newPort->write((char*)msg_to_send, REQUEST_NORMAL_LENGTH);
        std::cout << "Success" << std::endl;
        std::cout << "Closing port" << std::endl;
        newPort->close();
        newPort->deleteLater();
    }
    std::cout << "Goodbye!" << std::endl;
}

