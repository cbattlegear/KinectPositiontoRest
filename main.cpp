// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include <cpprest/http_client.h>

#include <k4a/k4a.h>
#include <k4abt.h>

#include <Windows.h>

#include "BodyTracking.h"
#include "BodyPosition.h"

using namespace std;

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features

#define VERIFY(result, error)                                                                            \
    if(result != K4A_RESULT_SUCCEEDED)                                                                   \
    {                                                                                                    \
        printf("%s \n - (File: %s, Function: %s, Line: %d)\n", error, __FILE__, __FUNCTION__, __LINE__); \
        exit(1);                                                                                         \
    }                                                                                                    \

void print_body_information(k4abt_body_t body)
{
    printf("Body ID: %u ", body.id);
	k4a_float3_t pelvisJoint = body.skeleton.joints[K4ABT_JOINT_PELVIS].position;
	printf("x:%f, y:%f, z:%f \n", pelvisJoint.v[0], pelvisJoint.v[1], pelvisJoint.v[2]);
}

void print_joint_information(k4a_float3_t position)
{
	printf("x:%f, y:%f, z:%f \n", position.v[0], position.v[1], position.v[2]);
}

int main()
{
    k4a_device_configuration_t device_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    device_config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;

    k4a_device_t device;
    VERIFY(k4a_device_open(0, &device), "Open K4A Device failed!");
    VERIFY(k4a_device_start_cameras(device, &device_config), "Start K4A cameras failed!");

    k4a_calibration_t sensor_calibration;
    VERIFY(k4a_device_get_calibration(device, device_config.depth_mode, K4A_COLOR_RESOLUTION_OFF, &sensor_calibration),
        "Get depth camera calibration failed!");

    k4abt_tracker_t tracker = NULL;
    VERIFY(k4abt_tracker_create(&sensor_calibration, &tracker), "Body tracker initialization failed!");

	// before entering update loop
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	GetConsoleScreenBufferInfo(h, &bufferInfo);

    int frame_count = 0;
    do
    {
		BodyTracking bods;
		SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
        k4a_capture_t sensor_capture;
        k4a_wait_result_t get_capture_result = k4a_device_get_capture(device, &sensor_capture, K4A_WAIT_INFINITE);
        if (get_capture_result == K4A_WAIT_RESULT_SUCCEEDED)
        {
            frame_count++;

            //printf("Start processing frame %d\n", frame_count);

            k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(tracker, sensor_capture, K4A_WAIT_INFINITE);

            k4a_capture_release(sensor_capture);
            if (queue_capture_result == K4A_WAIT_RESULT_TIMEOUT)
            {
                // It should never hit timeout when K4A_WAIT_INFINITE is set.
                printf("Error! Add capture to tracker process queue timeout!\n");
                break;
            }
            else if (queue_capture_result == K4A_WAIT_RESULT_FAILED)
            {
                printf("Error! Add capture to tracker process queue failed!\n");
                break;
            }

            k4abt_frame_t body_frame = NULL;
            k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(tracker, &body_frame, K4A_WAIT_INFINITE);
            if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
            {
                size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
                printf("%zu bodies are detected!\n", num_bodies);

                for (size_t i = 0; i < num_bodies; i++)
                {
                    k4abt_body_t body;
                    VERIFY(k4abt_frame_get_body_skeleton(body_frame, i, &body.skeleton), "Get body from body frame failed!");
                    body.id = k4abt_frame_get_body_id(body_frame, i);

					k4a_float3_t leftWristJoint = body.skeleton.joints[K4ABT_JOINT_WRIST_LEFT].position;
					k4a_float3_t rightWristJoint = body.skeleton.joints[K4ABT_JOINT_WRIST_RIGHT].position;
					k4a_float3_t headJoint = body.skeleton.joints[K4ABT_JOINT_HEAD].position;

					// Notice: y direction is pointing towards the ground! So jointA.y < jointB.y means jointA is higher than jointB
					bool bothHandsAreRaised = leftWristJoint.xyz.y < headJoint.xyz.y &&
						rightWristJoint.xyz.y < headJoint.xyz.y;

					k4a_float3_t pelvisJoint = body.skeleton.joints[K4ABT_JOINT_PELVIS].position;

					BodyPosition bodypos;

					bodypos.id = body.id;
					bodypos.x = pelvisJoint.xyz.x;
					bodypos.y = pelvisJoint.xyz.y;
					bodypos.z = pelvisJoint.xyz.z;

					bods.bodies.push_back(bodypos);

					if (bothHandsAreRaised) {
						//print_body_information(body);
						print_joint_information(pelvisJoint);
					}
                    
                }

                /* k4a_image_t body_index_map = k4abt_frame_get_body_index_map(body_frame);
                if (body_index_map != NULL)
                {
                    print_body_index_map_middle_line(body_index_map);
                    k4a_image_release(body_index_map);
                }
                else
                {
                    printf("Error: Fail to generate bodyindex map!\n");
                }*/

                k4abt_frame_release(body_frame);
            }
            else if (pop_frame_result == K4A_WAIT_RESULT_TIMEOUT)
            {
                //  It should never hit timeout when K4A_WAIT_INFINITE is set.
                printf("Error! Pop body frame result timeout!\n");
                break;
            }
            else
            {
                printf("Pop body frame result failed!\n");
                break;
            }
        }
        else if (get_capture_result == K4A_WAIT_RESULT_TIMEOUT)
        {
            // It should never hit time out when K4A_WAIT_INFINITE is set.
            printf("Error! Get depth frame time out!\n");
            break;
        }
        else
        {
            printf("Get depth capture returned error: %d\n", get_capture_result);
            break;
        }

		if (!bods.bodies.empty()) {
			string json = bods.JSONOut();

			http_client client(L"https://nicksneeze.azurewebsites.net/api/KinectDataSink?code=YkSFfHwQb7RTowi1otBoqJPYze3MgzM9NGsfSA9t5eFwooj9fABltA==");
			http_request requester;
			requester.set_body(json);
			requester.set_method(methods::POST);
			requester.headers().set_content_type(U("application/json"));
			client.request(requester);

			std::cout << json;
		}
		
		Sleep(1000);

    } while (true);

    printf("Finished body tracking processing!\n");

    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    k4a_device_stop_cameras(device);
    k4a_device_close(device);

    return 0;
}
